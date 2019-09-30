package ae.hitb.proctf.drone_racing.programming.languageUtils

import ae.hitb.proctf.drone_racing.programming.language.*

@Suppress("UNCHECKED_CAST")
private class Resolution(val program: Program) {
    val namedFunctions = (program.functionDeclarations + Intrinsic.resolvable)
            .groupBy { it.name }
            .mapValues { (_, v) -> v.associateBy { it.parameterNames.size } }

    fun resolveCallsInFunction(f: FunctionDeclaration): FunctionDeclaration =
            FunctionDeclaration(f.name, f.parameterNames, f.parameterTypes, f.returnType, resolveCallsInStatement(f.body))

    private fun findBySignature(name: String, nArgs: Int) = namedFunctions[name]?.let { it[nArgs] }

    private fun <T : Expression> resolveCallsIn(expression: Expression): T = when (expression) {
        is IntLiteral -> expression
        is Variable -> expression
        is FunctionCall -> {
            val resolvedArgs = expression.argumentExpressions.map<Expression, Expression> { resolveCallsIn(it) }
            if (expression.functionDeclaration is UnresolvedFunction) {
                val name = expression.functionDeclaration.name
                val nArgs = expression.functionDeclaration.dimensions
                val functionDeclaration = findBySignature(name, nArgs) ?: expression.functionDeclaration
                expression.copy(functionDeclaration, resolvedArgs)
            } else {
                expression.copy(argumentExpressions = resolvedArgs)
            }
        }
        is UnaryOperation -> expression.copy(resolveCallsIn(expression.operand))
        is BinaryOperation -> expression.copy(left = resolveCallsIn(expression.left),
                                              right = resolveCallsIn(expression.right))
        is StringLiteral -> expression
        is ArrayLiteral -> expression.copy(initializers = expression.initializers.map { resolveCallsIn<Expression>(it) })
    } as T

    private fun resolveCallsInStatement(s: Statement): Statement = when (s) {
        Pass -> Pass
        is AssignStatement -> s.copy(expression = resolveCallsIn(s.expression))
        is IfStatement -> s.copy(resolveCallsIn(s.condition), resolveCallsInStatement(s.trueBranch), resolveCallsInStatement(s.falseBranch))
        is WhileStatement -> s.copy(resolveCallsIn(s.condition), resolveCallsInStatement(s.body))
        is ChainStatement -> s.copy(resolveCallsInStatement(s.leftPart), resolveCallsInStatement(s.rightPart))
        is ReturnStatement -> s.copy(resolveCallsIn(s.expression))
        is FunctionCallStatement -> s.copy(resolveCallsIn(s.functionCall))
        is TryStatement -> s.copy(resolveCallsInStatement(s.body),
                         s.catchBranches.map { it.copy(body = resolveCallsInStatement(it.body)) },
                         resolveCallsInStatement(s.finallyStatement))
        is ThrowStatement -> s.copy(dataExpression = resolveCallsIn(s.dataExpression))
    }
}

fun resolveCalls(program: Program): Program {
    val resolutionP1 = Resolution(program)
    val mainP1 = resolutionP1.resolveCallsInFunction(program.mainFunction)
    val functionsP1 = program.functionDeclarations.filter { it !== program.mainFunction }
                              .map { resolutionP1.resolveCallsInFunction(it) } + mainP1
    return Program(functionsP1, mainP1)
}
