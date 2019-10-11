package ae.hitb.proctf.drone_racing

import kotlin.test.*

class CodeRunnerTests {
    // Explicit test
    @Test(expected = java.nio.file.NoSuchFileException::class)
    fun testRunningCode() {
        val codeRunner = CodeRunner()
        println(codeRunner.runCode("test.class", emptyMap(), "........."))
    }
}