package ae.hitb.proctf.drone_racing.programming.languageUtils

import ae.hitb.proctf.drone_racing.programming.language.*

open class LanguageVisitor {
    open fun visitStatement(statement: Statement): Any =
            when (statement) {
                Pass -> visitSkip(statement as Pass)
                is AssignStatement -> visitAssign(statement)
                is IfStatement -> visitIf(statement)
                is WhileStatement -> visitWhile(statement)
                is ChainStatement -> visitChain(statement)
                is ReturnStatement -> visitReturn(statement)
                is FunctionCallStatement -> visitFunctionCallStatement(statement)
                is TryStatement -> visitTry(statement)
                is ThrowStatement -> visitThrow(statement)
            }

    open fun visitFunctionCallStatement(functionCallStatement: FunctionCallStatement): Any =
            visitFunctionCall(functionCallStatement.functionCall)

    open fun visitReturn(returns: ReturnStatement): Any =
            visitExpression(returns.expression)

    open fun visitSkip(skip: Pass): Any = Unit

    open fun visitChain(statement: ChainStatement): Any {
        visitStatement(statement.leftPart)
        visitStatement(statement.rightPart)
        return Unit
    }

    open fun visitWhile(statement: WhileStatement): Any {
        visitExpression(statement.condition)
        visitStatement(statement.body)
        return Unit
    }

    open fun visitIf(statement: IfStatement): Any {
        visitExpression(statement.condition)
        visitStatement(statement.trueBranch)
        visitStatement(statement.falseBranch)
        return Unit
    }

    open fun visitAssign(assignStatement: AssignStatement): Any {
        visitVariable(assignStatement.variable)
        visitExpression(assignStatement.expression)
        return Unit
    }

    open fun visitTry(tries: TryStatement): Any {
        visitStatement(tries.body)
        tries.catchBranches.forEach {
            visitExceptionType(it.exceptionType)
            visitVariable(it.dataVariable)
            visitStatement(it.body)
        }
        visitStatement(tries.finallyStatement)
        return Unit
    }

    open fun visitThrow(throws: ThrowStatement): Any {
        visitExceptionType(throws.exceptionType)
        visitExpression(throws.dataExpression)
        return Unit
    }

    open fun visitExpression(expression: Expression): Any = when (expression) {
        is IntLiteral -> visitConst(expression)
        is StringLiteral -> visitStringLiteral(expression)
        is Variable -> visitVariable(expression)
        is Param -> visitParam(expression)
        is FunctionCall -> visitFunctionCall(expression)
        is UnaryOperation -> visitUnaryOperation(expression)
        is BinaryOperation -> visitBinaryOperation(expression)
    }

    open fun visitStringLiteral(stringLiteral: StringLiteral): Any = Unit

    open fun visitVariable(variable: Variable): Any = Unit

    open fun visitParam(param: Param): Any = Unit

    open fun visitConst(const: IntLiteral): Any = Unit

    open fun visitUnaryOperation(unaryOperation: UnaryOperation): Any {
        visitExpression(unaryOperation.operand)
        return Unit
    }

    open fun visitBinaryOperation(binaryOperation: BinaryOperation): Any {
        visitExpression(binaryOperation.left)
        visitExpression(binaryOperation.right)
        return Unit
    }

    open fun visitFunctionCall(functionCall: FunctionCall): Any {
        functionCall.argumentExpressions.forEach {
            visitExpression(it)
        }
        return Unit
    }

    open fun visitExceptionType(exceptionType: ExceptionType) = Unit
}