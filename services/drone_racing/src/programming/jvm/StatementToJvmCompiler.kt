package ae.hitb.proctf.drone_racing.programming.jvm

import ae.hitb.proctf.drone_racing.programming.Compiler
import ae.hitb.proctf.drone_racing.programming.language.Program
import ae.hitb.proctf.drone_racing.programming.stack.StatementToStackCompiler

class StatementToJvmCompiler : Compiler<Program, ByteArray> {
    private val statementToStackCompiler = StatementToStackCompiler()
    private val stackToJvmCompiler = StackToJvmCompiler()

    override fun compile(source: Program): ByteArray {
        val stackProgram = statementToStackCompiler.compile(source)
        return stackToJvmCompiler.compile(stackProgram)
    }
}