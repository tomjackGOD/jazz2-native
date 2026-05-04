import UIKit

@_silgen_name("ios_bridge_handle_key")
func ios_bridge_handle_key(_ keyCode: Int32, _ isDown: Bool)

class TouchOverlayView: UIView {
    // These constants correspond to the nCine::Keys enum values in Keys.h
    private enum Keys: Int32 {
        case space = 4      // nCine::Keys::Space
        case x = 49         // nCine::Keys::X
        case z = 51         // nCine::Keys::Z
        case up = 70        // nCine::Keys::Up
        case down = 71      // nCine::Keys::Down
        case right = 72     // nCine::Keys::Right
        case left = 73      // nCine::Keys::Left
        case lShift = 98    // nCine::Keys::LShift
        case escape = 3     // nCine::Keys::Escape
    }

    private var leftButton: UIButton!
    private var rightButton: UIButton!
    private var upButton: UIButton!
    private var downButton: UIButton!
    
    private var jumpButton: UIButton!
    private var fireButton: UIButton!
    private var runButton: UIButton!
    private var changeButton: UIButton!
    private var pauseButton: UIButton!

    override init(frame: CGRect) {
        super.init(frame: frame)
        setupControls()
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setupControls()
    }

    private func setupControls() {
        backgroundColor = .clear
        isUserInteractionEnabled = true

        // D-Pad
        leftButton = createButton(title: "", systemImage: "arrow.left.circle", key: .left)
        rightButton = createButton(title: "", systemImage: "arrow.right.circle", key: .right)
        upButton = createButton(title: "", systemImage: "arrow.up.circle", key: .up)
        downButton = createButton(title: "", systemImage: "arrow.down.circle", key: .down)

        // Action Buttons
        jumpButton = createButton(title: "Jump", key: .z)
        fireButton = createButton(title: "Fire", key: .x)
        runButton = createButton(title: "Run", key: .lShift)
        changeButton = createButton(title: "Weapon", key: .space)
        pauseButton = createButton(title: "", systemImage: "pause.circle", key: .escape)

        addSubview(leftButton)
        addSubview(rightButton)
        addSubview(upButton)
        addSubview(downButton)
        addSubview(jumpButton)
        addSubview(fireButton)
        addSubview(runButton)
        addSubview(changeButton)
        addSubview(pauseButton)
    }

    private func createButton(title: String, systemImage: String? = nil, key: Keys) -> UIButton {
        let button = UIButton(type: .custom)
        if let systemImage = systemImage {
            let config = UIImage.SymbolConfiguration(pointSize: 30, weight: .bold)
            let image = UIImage(systemName: systemImage, withConfiguration: config)
            button.setImage(image, for: .normal)
            button.tintColor = .white
        } else {
            button.setTitle(title, for: .normal)
            button.setTitleColor(.white, for: .normal)
            button.titleLabel?.font = .boldSystemFont(ofSize: 18)
        }
        
        button.backgroundColor = UIColor.white.withAlphaComponent(0.2)
        button.layer.cornerRadius = 30
        button.layer.borderWidth = 1.5
        button.layer.borderColor = UIColor.white.withAlphaComponent(0.4).cgColor
        
        button.tag = Int(key.rawValue)
        
        button.addTarget(self, action: #selector(buttonDown(_:)), for: .touchDown)
        button.addTarget(self, action: #selector(buttonUp(_:)), for: [.touchUpInside, .touchUpOutside, .touchCancel])
        
        return button
    }

    @objc private func buttonDown(_ sender: UIButton) {
        sender.backgroundColor = UIColor.white.withAlphaComponent(0.6)
        ios_bridge_handle_key(Int32(sender.tag), true)
    }

    @objc private func buttonUp(_ sender: UIButton) {
        sender.backgroundColor = UIColor.white.withAlphaComponent(0.3)
        ios_bridge_handle_key(Int32(sender.tag), false)
    }

    override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
        for subview in subviews {
            if !subview.isHidden && subview.isUserInteractionEnabled && subview.point(inside: convert(point, to: subview), with: event) {
                return true
            }
        }
        return false
    }

    override func layoutSubviews() {
        super.layoutSubviews()
        
        let size: CGFloat = 60
        let margin: CGFloat = 20
        let bottomY = bounds.height - size - margin
        
        // D-Pad Layout
        leftButton.frame = CGRect(x: margin, y: bottomY - size/2, width: size, height: size)
        rightButton.frame = CGRect(x: margin + size * 1.5, y: bottomY - size/2, width: size, height: size)
        upButton.frame = CGRect(x: margin + size * 0.75, y: bottomY - size * 1.25, width: size, height: size)
        downButton.frame = CGRect(x: margin + size * 0.75, y: bottomY + size * 0.25, width: size, height: size)

        // Action Buttons Layout
        let rightX = bounds.width - size - margin
        jumpButton.frame = CGRect(x: rightX, y: bottomY, width: size * 1.5, height: size)
        fireButton.frame = CGRect(x: rightX - size * 1.7, y: bottomY, width: size * 1.5, height: size)
        runButton.frame = CGRect(x: rightX, y: bottomY - size * 1.2, width: size * 1.5, height: size)
        changeButton.frame = CGRect(x: rightX - size * 1.7, y: bottomY - size * 1.2, width: size * 1.5, height: size)
        
        pauseButton.frame = CGRect(x: bounds.width - size - margin, y: margin, width: size, height: size)
    }
}
