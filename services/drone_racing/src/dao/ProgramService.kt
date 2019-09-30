package ae.hitb.proctf.drone_racing.dao

import ae.hitb.proctf.drone_racing.programming.jvm.StatementToJvmCompiler
import ae.hitb.proctf.drone_racing.programming.parsing.readProgram
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.jetbrains.exposed.sql.and
import java.nio.file.*

const val PROGRAMS_PATH = "./programs"
const val PROGRAM_FILENAME_LENGTH = 20

class ProgramService {
    suspend fun findProgramById(id: Int) = dbQuery {
        Program.findById(id)
    }

    suspend fun findUserPrograms(user: User) = dbQuery {
        Program.find { Programs.author eq user.id }.toList()
    }

    suspend fun findUserPrograms(user: User, level: Level) = dbQuery {
        Program.find { (Programs.author eq user.id) and (Programs.level eq level.id) }.toList()
    }

    private fun ensureDirectoryExists() {
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
        ensureDirectoryExists()
        val program = readProgram(sourceCode)
        val compiler = StatementToJvmCompiler()
        val bytecode = compiler.compile(title, program)
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
}