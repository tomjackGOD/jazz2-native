import UIKit

@_silgen_name("ios_bridge_run")
func ios_bridge_run()

@_silgen_name("ios_bridge_suspend")
func ios_bridge_suspend()

@_silgen_name("ios_bridge_resume")
func ios_bridge_resume()

@UIApplicationMain
class AppDelegate: UIResponder, UIApplicationDelegate {
    var window: UIWindow?

    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        window = UIWindow(frame: UIScreen.main.bounds)
        let viewController = ViewController()
        window?.rootViewController = viewController
        window?.makeKeyAndVisible()
        
        // Start the game loop
        ios_bridge_run()
        
        return true
    }

    func applicationWillResignActive(_ application: UIApplication) {
        ios_bridge_suspend()
    }

    func applicationDidEnterBackground(_ application: UIApplication) {
    }

    func applicationWillEnterForeground(_ application: UIApplication) {
    }

    func applicationDidBecomeActive(_ application: UIApplication) {
        ios_bridge_resume()
    }

    func applicationWillTerminate(_ application: UIApplication) {
    }
}
