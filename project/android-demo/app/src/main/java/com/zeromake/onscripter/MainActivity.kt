package com.zeromake.onscripter

import org.libsdl.app.SDLActivity

class MainActivity : SDLActivity() {
    override fun getLibraries(): Array<String> {
        return arrayOf("main")
    }
}