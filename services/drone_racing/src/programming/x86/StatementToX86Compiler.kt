package ae.hitb.proctf.drone_racing.programming.x86

import ae.hitb.proctf.drone_racing.programming.Compiler
import ae.hitb.proctf.drone_racing.programming.language.Program
import ae.hitb.proctf.drone_racing.programming.stack.StatementToStackCompiler

class StatementToX86Compiler(private val targetPlatform: TargetPlatform) : Compiler<Program, String> {
    private val statementToStackCompiler = StatementToStackCompiler()
    private val stackToX86Compiler = StackToX86Compiler(targetPlatform)

    override fun compile(source: Program): String {
        val stackProgram = statementToStackCompiler.compile(source)
        return stackToX86Compiler.compile(stackProgram)
    }
}