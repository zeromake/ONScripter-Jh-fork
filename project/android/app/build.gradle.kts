import java.util.Properties
import java.io.FileInputStream

plugins {
    id("com.android.application")
}

val keystorePropertiesFile = rootProject.file("keystore.properties")
var keystoreProperties: Properties? = null
if (keystorePropertiesFile.exists()) {
    keystoreProperties = Properties()
    keystoreProperties!!.load(FileInputStream(keystorePropertiesFile))
}

val localPropertiesFile = rootProject.file("local.properties")
var localVersionName: String? = null
if (localPropertiesFile.exists()) {
    val localProperties = Properties()
    localProperties.load(FileInputStream(localPropertiesFile))
    localProperties.let { it ->
        val versionName: String? = it["versionName"] as String?
        versionName?.let {
            localVersionName = it
        }
    }
}

android {
    namespace = "com.zeromake.onscripter"
    compileSdk = 34
    defaultConfig {
        applicationId = "com.zeromake.onscripter"
        minSdk = 16
        versionCode = 1
        versionName = "1.0.0"
        localVersionName?.let {
            versionCode = (it.replace(",", "", false)).toIntOrNull() ?: 1
            versionName = it
        }
    }
    signingConfigs {
        keystoreProperties?.let {
            create("config") {
                keyAlias = it["keyAlias"] as String?
                keyPassword = it["keyPassword"] as String?
                storeFile = file((it["storeFile"] as String).replace("\${root}", rootProject.projectDir.absolutePath, false))
                storePassword = it["storePassword"] as String?
            }
        }
    }
    buildTypes {
        release {
            isMinifyEnabled = false
            signingConfig = signingConfigs.findByName("config")
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            isDebuggable = false
        }
        debug {
            signingConfig = signingConfigs.getByName("debug")
            isDebuggable = true
            applicationIdSuffix = ".debug"
            versionNameSuffix = "-debug"
        }
    }
    sourceSets {
        getByName("main") {
            // armeabi-v7a,arm64-v8a,x86_64
            jniLibs.srcDirs("libs")
        }
    }
}

dependencies {
    implementation(project(":XXPermissions"))
//    implementation("com.android.support:support-annotations:28.0.0")
//    implementation("androidx.core:core:1.9.0")
//    implementation("com.squareup.picasso:picasso:2.8")
    implementation("com.nostra13.universalimageloader:universal-image-loader:1.9.5")
//    implementation("com.facebook.fresco:fresco:2.6.0")
}
