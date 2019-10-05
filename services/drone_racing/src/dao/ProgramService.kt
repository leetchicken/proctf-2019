package ae.hitb.proctf.drone_racing.dao

import ae.hitb.proctf.drone_racing.programming.jvm.StatementToJvmCompiler
import ae.hitb.proctf.drone_racing.programming.parsing.readProgram
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.jetbrains.exposed.dao.with
import org.jetbrains.exposed.sql.and
import java.nio.file.*

const val PROGRAMS_PATH = "./programs"
const val PROGRAM_FILENAME_LENGTH = 20

@ExperimentalStdlibApi
class ProgramService {
    private val compiler = StatementToJvmCompiler()

    suspend fun findProgramById(id: Int) = dbQuery {
        Program.findById(id)
    }

    suspend fun findUserPrograms(user: User) = dbQuery {
        Program.find { Programs.author eq user.id }.toList()
    }

    fun findUserPrograms(user: User, level: Level): List<Program> {
        return Program.find { (Programs.author eq user.id) and (Programs.level eq level.id) }.with(Program::author).toList()
    }

    private fun ensureProgramsDirectoryExists() {
        val path = Paths.get(PROGRAMS_PATH)
        if (! Files.exists(path))
            Files.createDirectory(path)
    }

    private val filenameCharPool : List<Char> = ('a'..'z') + ('A'..'Z') + ('0'..'9')

    private fun generateAlphanumeric(length: Int): String {
        return (1..length)
            .map { _ -> kotlin.random.Random.nextInt(0, filenameCharPool.size) }
            .map(filenameCharPool::get)
            .joinToString("");
    }

    suspend fun createProgram(author: User, level: Level, title: String, sourceCode: String): Program {
        ensureProgramsDirectoryExists()

        val program = readProgram(sourceCode)
        val bytecode = replaceClassName(compiler.compile(program), "GeneratedClass", title)
        val filename = generateAlphanumeric(PROGRAM_FILENAME_LENGTH) + ".class"
        val path = Paths.get(PROGRAMS_PATH, filename)

        withContext(Dispatchers.IO) {
            Files.write(path, bytecode)
        }

        return dbQuery {
            Program.new {
                this.author = author
                this.level = level
                this.title = title
                this.file = filename
            }
        }
    }

    private fun replaceClassName(classBytes: ByteArray, currentClassName: String, newClassName: String): ByteArray {
        fun isSubsequenceEqual(first: ByteArray, firstIndex: Int, second: ByteArray, secondIndex: Int, length: Int): Boolean {
            val f = first.drop(firstIndex).take(length)
            val s = second.drop(secondIndex).take(length)
            return f.zip(s).all { (b1, b2) -> b1 == b2 }
        }

        fun findPosition(byteArray: ByteArray, subsequence: ByteArray): Pair<Int, Int>? {
            byteArray.indices.forEach { index ->
                if (isSubsequenceEqual(byteArray, index, subsequence, 0, subsequence.size))
                    return Pair(index, index + subsequence.size)
            }
            return null
        }

        fun placeBytes(bytes: ByteArray, startIndex: Int, finishIndex: Int, newBytes: ByteArray): ByteArray {
            return bytes.take(startIndex).toByteArray() + newBytes + bytes.drop(finishIndex)
        }

        check(newClassName.length in 1..currentClassName.length) { "Title should be not empty and not longer than ${currentClassName.length} chars" }
        var className = newClassName
        while (className.length < currentClassName.length)
            className += "\u0000"

        val position = findPosition(classBytes, currentClassName.encodeToByteArray()) ?: return classBytes

        return placeBytes(classBytes, position.first, position.second, className.encodeToByteArray())
    }
}