import UIKit
import Metal

@_silgen_name("ios_bridge_process_frame")
func ios_bridge_process_frame(_ deltaTime: Float)

@_silgen_name("ios_bridge_handle_resize")
func ios_bridge_handle_resize(_ width: Int32, _ height: Int32)

@_silgen_name("ios_bridge_handle_touch")
func ios_bridge_handle_touch(_ type: Int32, _ x: Int32, _ y: Int32, _ pointerId: Int32, _ pressure: Float, _ majorRadius: Float)

@_silgen_name("ios_bridge_set_metal_layer")
func ios_bridge_set_metal_layer(_ layer: UnsafeMutableRawPointer)

class ViewController: UIViewController {
    private enum TouchEventType: Int32 {
        case began = 0
        case moved = 1
        case ended = 2
        case cancelled = 3
    }

    private var metalLayer: CAMetalLayer?
    private var displayLink: CADisplayLink?
    private var touchOverlay: TouchOverlayView?

    override func viewDidLoad() {
        super.viewDidLoad()
        
        setupMetalLayer()
        setupTouchOverlay()
        
        // Start the game loop after the view has loaded
        startDisplayLink()
    }
    
    private func setupTouchOverlay() {
        touchOverlay = TouchOverlayView(frame: view.bounds)
        if let overlay = touchOverlay {
            overlay.autoresizingMask = [.flexibleWidth, .flexibleHeight]
            view.addSubview(overlay)
        }
    }
    
    private func setupMetalLayer() {
        metalLayer = CAMetalLayer()
        metalLayer?.frame = view.bounds
        metalLayer?.isOpaque = true
        metalLayer?.device = MTLCreateSystemDefaultDevice()
        metalLayer?.pixelFormat = .bgra8Unorm
		
        let scale = UIScreen.main.scale
        metalLayer?.contentsScale = scale
        metalLayer?.drawableSize = CGSize(width: view.bounds.width * scale, height: view.bounds.height * scale)
        
        if let layer = metalLayer {
            view.layer.addSublayer(layer)
            
            // Pass the metal layer to C++ side
            let layerPointer = Unmanaged.passUnretained(layer).toOpaque()
            ios_bridge_set_metal_layer(layerPointer)
        }
    }
    
    private func startDisplayLink() {
        displayLink = CADisplayLink(target: self, selector: #selector(renderLoop))
        displayLink?.add(to: .main, forMode: .common)
    }
    
    @objc private func renderLoop() {
        let duration = displayLink?.duration ?? 0
        let deltaTime = max(duration > 0 ? Float(duration) : Float(1.0/60.0), Float(0.001))
        ios_bridge_process_frame(deltaTime)
    }
    
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let point = touch.location(in: view)
            let scale = metalLayer?.contentsScale ?? UIScreen.main.scale
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
            let scale = metalLayer?.contentsScale ?? UIScreen.main.scale
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
            let scale = metalLayer?.contentsScale ?? UIScreen.main.scale
            let radiusPx = Float(touch.majorRadius * scale)
            ios_bridge_handle_touch(TouchEventType.ended.rawValue, Int32(point.x * scale), Int32(point.y * scale), Int32(touch.hash), 0.0, radiusPx)
        }
    }
    
    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let point = touch.location(in: view)
            let scale = metalLayer?.contentsScale ?? UIScreen.main.scale
            let radiusPx = Float(touch.majorRadius * scale)
            ios_bridge_handle_touch(TouchEventType.cancelled.rawValue, Int32(point.x * scale), Int32(point.y * scale), Int32(touch.hash), 0.0, radiusPx)
        }
    }
    
    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        metalLayer?.frame = view.bounds
        
        // Keep drawable size in sync after layout changes (rotation, split view, etc.)
        let scale = metalLayer?.contentsScale ?? UIScreen.main.scale
        metalLayer?.drawableSize = CGSize(width: view.bounds.width * scale, height: view.bounds.height * scale)
        
        // Notify C++ side of resize
        let width = Int32(view.bounds.width * scale)
        let height = Int32(view.bounds.height * scale)
        ios_bridge_handle_resize(width, height)
    }
}
