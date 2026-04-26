import Foundation
import UIKit

private enum LanguageManager {
    // Fixed-size C buffer with stable storage for returning an UnsafePointer
    // across the Swift <-> C++ boundary.
    static let capacity = 16
    static let languageBuffer: UnsafeMutablePointer<Int8> = {
        let ptr = UnsafeMutablePointer<Int8>.allocate(capacity: capacity)
        ptr.initialize(repeating: 0, count: capacity)
        return ptr
    }()
}

@_cdecl("ios_bridge_get_preferred_language")
public func iosBridgeGetPreferredLanguage() -> UnsafePointer<Int8>? {
    if let languageCode = Locale.current.languageCode {
        let fullCode: String
        if let regionCode = Locale.current.regionCode {
            fullCode = "\(languageCode)-\(regionCode)"
        } else {
            fullCode = languageCode
        }
        
        let cString = fullCode.cString(using: .utf8) ?? []
        let maxCopy = min(cString.count, LanguageManager.capacity - 1)
        if maxCopy > 0 {
            for i in 0..<maxCopy {
                LanguageManager.languageBuffer[i] = cString[i]
            }
        }
        LanguageManager.languageBuffer[maxCopy] = 0

        return UnsafePointer(LanguageManager.languageBuffer)
    }
    return nil
}

@_cdecl("ios_bridge_is_screen_round")
public func iosBridgeIsScreenRound() -> Bool {
    return false
}

@_cdecl("ios_bridge_has_external_storage_permission")
public func iosBridgeHasExternalStoragePermission() -> Bool {
    // iOS apps are sandboxed and always have access to their own Documents directory.
    // There is no system-level "External Storage" permission as on Android.
    return true
}

@_cdecl("ios_bridge_request_external_storage_permission")
public func iosBridgeRequestExternalStoragePermission() {
}

@_cdecl("ios_bridge_open_url")
public func iosBridgeOpenUrl(_ urlStringPointer: UnsafePointer<Int8>) -> Bool {
    let urlString = String(cString: urlStringPointer)
    guard let url = URL(string: urlString) else { return false }
    if UIApplication.shared.canOpenURL(url) {
        UIApplication.shared.open(url, options: [:], completionHandler: nil)
        return true
    }
    return false
}

@_cdecl("ios_bridge_get_screen_scale")
public func iosBridgeGetScreenScale() -> Float {
    return Float(UIScreen.main.scale)
}

@_cdecl("ios_bridge_get_screen_width")
public func iosBridgeGetScreenWidth() -> Int32 {
    return Int32(UIScreen.main.bounds.size.width * UIScreen.main.scale)
}

@_cdecl("ios_bridge_get_screen_height")
public func iosBridgeGetScreenHeight() -> Int32 {
    return Int32(UIScreen.main.bounds.size.height * UIScreen.main.scale)
}
