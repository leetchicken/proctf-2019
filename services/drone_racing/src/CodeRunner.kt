package ae.hitb.proctf.drone_racing

import ae.hitb.proctf.drone_racing.dao.PROGRAMS_PATH
import java.lang.reflect.InvocationTargetException
import java.nio.file.*
import kotlin.concurrent.thread

const val TIME_LIMIT_MILLISECONDS = 100

class CodeRunner {
    fun runCode(classFilename: String, params: Map<String, String>, maze: String): RunResult {
        val classBytes = Files.readAllBytes(Paths.get(PROGRAMS_PATH, classFilename))
        val classLoader = ByteArrayClassLoader(mapOf("Code" to classBytes))
        var movesCount= Int.MAX_VALUE
        var success = false
        var exception: Throwable? = null

        setParams(params);
        try {
            val codeThread = thread(start = false, isDaemon = true) {
                try {
                    val klass = classLoader.loadClass("Code")

                    val setMaze = klass.getMethod("setMaze", String::class.java)
                    setMaze.invoke(null, maze)

                    val main = klass.getMethod("main")
                    main.invoke(null)

                    val getMovesCount = klass.getMethod("getMovesCount")
                    movesCount = getMovesCount.invoke(null) as Int

                    val isOnTopRightCell = klass.getMethod("isOnTopRightCell")
                    success = isOnTopRightCell.invoke(null) as Boolean
                } catch (e: Throwable) {
                    e.printStackTrace()
                    exception = e
                }
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
                return RunResult(
                    success = false,
                    error = true,
                    errorMessage = "Your code has been interrupted because it needs more than $TIME_LIMIT_MILLISECONDS milliseconds"
                )
            }

            println("Finish")
        } finally {
            clearParams(params)
        }

        if (exception != null)
            return RunResult(
                success = false,
                error = true,
                errorMessage = getErrorMessage(exception)
            )

        return RunResult(
            success = success,
            score = movesCount
        )
    }

    private fun getErrorMessage(exception: Throwable?): String {
        val defaultErrorMessage = "Exception occurred"
        if (exception == null)
            return defaultErrorMessage
        return when (exception) {
            is InvocationTargetException -> exception.cause?.message ?: exception.message ?: defaultErrorMessage
            else -> exception.message ?: defaultErrorMessage
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

data class RunResult(val success: Boolean, val score: Int = 0, val error: Boolean = false, val errorMessage: String = "")


class ByteArrayClassLoader(private val classBytes: Map<String, ByteArray>): ClassLoader() {
    override fun findClass(name: String?): Class<*> {
        if (classBytes.containsKey(name)) {
            val thisClassBytes = classBytes[name]!!
            return defineClass(null, thisClassBytes, 0, thisClassBytes.size)
        }
        return super.findClass(name)
    }
}