package ae.hitb.proctf.drone_racing

import kotlin.test.*

class CodeRunnerTests {
    @Test
    fun testRunningCode() {
        val codeRunner = CodeRunner()
        codeRunner.runCode("test.class", emptyMap())
    }
}