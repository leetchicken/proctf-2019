package ae.hitb.proctf.drone_racing

import ae.hitb.proctf.drone_racing.dao.PROGRAMS_PATH
import java.nio.file.*
import kotlin.concurrent.thread

const val TIME_LIMIT_MILLISECONDS = 100

class CodeRunner {
    fun runCode(classFilename: String, params: Map<String, String>, maze: String) {
        val classBytes = Files.readAllBytes(Paths.get(PROGRAMS_PATH, classFilename))
        val classLoader = ByteArrayClassLoader(mapOf("Code" to classBytes))

        setParams(params);
        try {
            val codeThread = thread(start = false, isDaemon = true) {
                val klass = classLoader.loadClass("Code")
                // TODO
                // call klass.setMaze(maze)
                // call klass.main()
                val main = klass.getMethod("main", Array<String>::class.java)
                main.invoke(null, emptyArray<String>())
            }
            try {
                codeThread.start()
            } catch (e: Throwable) {
                e.printStackTrace()
            }

            val startTime = System.currentTimeMillis()
            while (System.currentTimeMillis() - startTime < TIME_LIMIT_MILLISECONDS) {
                if (! codeThread.isAlive) {
                    println("Thread is dead. Welcome the thread!")
                    break
                }
                println("Continue checking")
                Thread.sleep(10)
            }
            if (codeThread.isAlive) {
                println("It's toooooo long. I'll kill it")
                try {
                    codeThread.interrupt()
                } catch (e: Throwable) {
                    e.printStackTrace()
                }
            }

            println("Finish")
        } finally {
            clearParams(params)
        }
    }

    private fun setParams(params: Map<String, String>) {
        for (param in params)
            System.setProperty(param.key, param.value)
    }

    private fun clearParams(params: Map<String, String>) {
        for (param in params)
            System.setProperty(param.key, "")
    }
}

data class RunResult(val success: Boolean, val error: Boolean, val errorMessage: String)


class ByteArrayClassLoader(private val classBytes: Map<String, ByteArray>): ClassLoader() {
    override fun findClass(name: String?): Class<*> {
        if (classBytes.containsKey(name)) {
            val thisClassBytes = classBytes[name]!!
            return defineClass(null, thisClassBytes, 0, thisClassBytes.size)
        }
        return super.findClass(name)
    }
}