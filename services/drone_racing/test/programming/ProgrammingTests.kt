package ae.hitb.proctf.drone_racing.programming

import ae.hitb.proctf.drone_racing.programming.jvm.StackToJvmCompiler
import ae.hitb.proctf.drone_racing.programming.language.Program
import ae.hitb.proctf.drone_racing.programming.parsing.*
import ae.hitb.proctf.drone_racing.programming.stack.StatementToStackCompiler
import java.io.File
import java.nio.charset.Charset
import kotlin.test.*

class ProgrammingTests {
    @ExperimentalUnsignedTypes
    @Test
    fun parsingTest() {
        val program: Program = readProgram("""
fun getSum(x: int): int 
begin
   return x + 10
end
            
sum = 0
for i = 1..11-1 do begin
   sum = sum + i
end
a = 100
write(to_string(getSum(a)))
write(to_string(sum))
""");

        compileAndRunProgram(program)
    }

    @ExperimentalUnsignedTypes
    @Test
    fun compilationTest() {
        val program = readProgram("""
sum = 0
for i = 1 .. 10 do begin
    sum = sum + i
end
write(to_string(sum))
write(to_string(2 * sum))
write(to_string(strlen("hello")))
if strcmp("hello", "world") < 0 then
    write("Yes")
end

""")

       compileAndRunProgram(program)
    }

    @ExperimentalUnsignedTypes
    @ExperimentalStdlibApi
    @Test
    fun returnStrTest() {
        val program = readProgram("""
fun hello(): str
begin
    return "hello world"
end

write(hello())
""")
        compileAndRunProgram(program)
    }

    @ExperimentalUnsignedTypes
    @Test
    fun functionTest() {
        val program = readProgram("""
fun print_number(x: int)
begin
   write(to_string(x))
   write(to_string(2 * x))
   write("Yes")
   local = 10
   write(to_string(local + 100500))
end

fun empty_function(x: int)
begin
end

fun get_double(x: int): int
begin
    return 2 * x
end

fun set_maze(s: str)
begin
  
end

print_number(1)
print_number(get_double(100))
""")
        compileAndRunProgram(program)
    }

    @ExperimentalUnsignedTypes
    @Test(expected = java.lang.reflect.InvocationTargetException::class)
    fun hackTest() {
        val program = readProgram("""
write("org/h2/tools/Server")
write("MyProgram")

openBrowser("yandex.ru")
""")
        System.getProperties().forEach {
            println(it)
        }
        compileAndRunProgram(program)
    }

    @ExperimentalUnsignedTypes
    @Test
    fun paramTest() {
        val program = readProgram("write(\$n)")
        System.setProperty("n", "HELLO WORLD")
        compileAndRunProgram(program)
    }

    @ExperimentalUnsignedTypes
    private fun compileAndRunProgram(program: Program) {
        val stackCompiler = StatementToStackCompiler()
        val stackProgram = stackCompiler.compile(program)

        stackProgram.functions.forEach {
            println("========= " + it.key.name.toUpperCase() + " ===========")
            println(it.value.joinToString(separator = "\n"))
        }

        val jvmCompiler = StackToJvmCompiler()
        val jvmBytecode = jvmCompiler.compile(stackProgram)
        println(jvmBytecode.joinToString(separator = " ") { it.toUByte().toString() })

        File("Program.class").writeBytes(jvmBytecode)

        val classLoader = ByteArrayClassLoader(mapOf("Program" to jvmBytecode))
        val klass = classLoader.loadClass("Program")
        klass.getMethod("setMaze", String::class.java).invoke(null, "....")
        val main = klass.getMethod("main")
        main.invoke(null)
    }

    private fun byteArrayOfInts(vararg ints: Int) = ByteArray(ints.size) { pos -> ints[pos].toByte() }

    @ExperimentalUnsignedTypes
    @Test
    fun conversionTest() {
        println("Привет".length)
        println("Привет".toByteArray(Charset.forName("utf-8")).size)
        println("Привет".toByteArray(Charset.forName("utf-8")).map { it.toUByte().toString(16) })
        println("Привет".toByteArray().size)

        val byteList = byteArrayOfInts(
            0xd0, 0x9f, 0xd1, 0x80, 0xd0, 0xb8, 0xd0, 0xb2, 0xd0, 0xb5, 0xd1, 0x82,
            0x50, 0x72, 0x6F, 0x67, 0x72, 0x61, 0x6D,
            0x07, 0x00, 0x01,
            0x01, 0x00, 0x13,
            0x6F, 0x72, 0x67, 0x2F, 0x68, 0x32, 0x2F, 0x74, 0x6F, 0x6F, 0x6C, 0x73, 0x2F, 0x53, 0x65, 0x72, 0x76, 0x65, 0x72)
        println(byteList.toString(Charset.forName("UTF-8")).length)
        val s = String(byteList, Charset.forName("utf-8"))
        println(s.length)
        println(s)
    }
}

class ByteArrayClassLoader(private val classBytes: Map<String, ByteArray>): ClassLoader() {
    override fun findClass(name: String?): Class<*> {
        if (classBytes.containsKey(name)) {
            val thisClassBytes = classBytes[name]!!
            return defineClass(null, thisClassBytes, 0, thisClassBytes.size)
        }
        return super.findClass(name)
    }
}