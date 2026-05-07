import UIKit
import Metal
import MetalKit
import QuartzCore

@_silgen_name("ios_bridge_process_frame")
func ios_bridge_process_frame(_ deltaTime: Float)

@_silgen_name("ios_bridge_handle_resize")
func ios_bridge_handle_resize(_ width: Int32, _ height: Int32)

@_silgen_name("ios_bridge_handle_touch")
func ios_bridge_handle_touch(_ type: Int32, _ x: Int32, _ y: Int32, _ pointerId: Int32, _ pressure: Float, _ majorRadius: Float)

@_silgen_name("ios_bridge_set_metal_layer")
func ios_bridge_set_metal_layer(_ layer: UnsafeMutableRawPointer)

class ViewController: UIViewController, MTKViewDelegate {
    private enum TouchEventType: Int32 {
        case began = 0
        case moved = 1
        case ended = 2
        case cancelled = 3
    }

    private var metalView: MTKView?
    private var touchOverlay: TouchOverlayView?
    private var lastRenderTime: CFTimeInterval = 0
    private var hasSetMetalLayer: Bool = false

    override func viewDidLoad() {
        super.viewDidLoad()
        
        setupMetalView()
        setupTouchOverlay()
    }
    
    private func setupTouchOverlay() {
        touchOverlay = TouchOverlayView(frame: view.bounds)
        if let overlay = touchOverlay {
            overlay.autoresizingMask = [.flexibleWidth, .flexibleHeight]
            view.addSubview(overlay)
        }
    }
    
    private func setupMetalView() {
        metalView = MTKView(frame: view.bounds)
        metalView?.device = MTLCreateSystemDefaultDevice()
        metalView?.colorPixelFormat = .bgra8Unorm
        metalView?.isOpaque = true
        metalView?.enableSetNeedsDisplay = false
        metalView?.preferredFramesPerSecond = 60
        metalView?.delegate = self
        
        let scale = UIScreen.main.scale
        metalView?.contentScaleFactor = scale
        
        if let mtkView = metalView {
            mtkView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
            view.addSubview(mtkView)
        }
    }
    
    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        let width = Int32(size.width)
        let height = Int32(size.height)
        ios_bridge_handle_resize(width, height)
    }
    
    func draw(in view: MTKView) {
        // Pass the metal layer to C++ side on first draw
        if !hasSetMetalLayer, let drawable = view.currentDrawable {
            let layer = drawable.layer
            let layerPointer = Unmanaged.passUnretained(layer).toOpaque()
            ios_bridge_set_metal_layer(layerPointer)
            hasSetMetalLayer = true
        }
        
        let currentTime = CACurrentMediaTime()
        let deltaTime = lastRenderTime > 0 ? Float(currentTime - lastRenderTime) : Float(1.0 / 60.0)
        lastRenderTime = currentTime
        
        ios_bridge_process_frame(max(deltaTime, Float(0.001)))
    }
    
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let point = touch.location(in: view)
            let scale = metalView?.contentScaleFactor ?? UIScreen.main.scale
            let normalizedForce: Float
            if touch.maximumPossibleForce > 0.0 {
                normalizedForce = Float(touch.force / touch.maximumPossibleForce)
            } else {
                normalizedForce = 0.0
            }
            let radiusPx = Float(touch.majorRadius * scale)
            ios_bridge_handle_touch(TouchEventType.began.rawValue, Int32(point.x * scale), Int32(point.y * scale), Int32(touch.hash), normalizedForce, radiusPx)
        }
    }
    
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let point = touch.location(in: view)
            let scale = metalView?.contentScaleFactor ?? UIScreen.main.scale
            let normalizedForce: Float
            if touch.maximumPossibleForce > 0.0 {
                normalizedForce = Float(touch.force / touch.maximumPossibleForce)
            } else {
                normalizedForce = 0.0
            }
            let radiusPx = Float(touch.majorRadius * scale)
            ios_bridge_handle_touch(TouchEventType.moved.rawValue, Int32(point.x * scale), Int32(point.y * scale), Int32(touch.hash), normalizedForce, radiusPx)
        }
    }
    
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let point = touch.location(in: view)
            let scale = metalView?.contentScaleFactor ?? UIScreen.main.scale
            let radiusPx = Float(touch.majorRadius * scale)
            ios_bridge_handle_touch(TouchEventType.ended.rawValue, Int32(point.x * scale), Int32(point.y * scale), Int32(touch.hash), 0.0, radiusPx)
        }
    }
    
    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let point = touch.location(in: view)
            let scale = metalView?.contentScaleFactor ?? UIScreen.main.scale
            let radiusPx = Float(touch.majorRadius * scale)
            ios_bridge_handle_touch(TouchEventType.cancelled.rawValue, Int32(point.x * scale), Int32(point.y * scale), Int32(touch.hash), 0.0, radiusPx)
        }
    }
}
