package ae.hitb.proctf.drone_racing.programming.parsing

import com.github.h0tk3y.betterParse.combinators.*
import com.github.h0tk3y.betterParse.grammar.Grammar
import com.github.h0tk3y.betterParse.grammar.parseToEnd
import com.github.h0tk3y.betterParse.grammar.parser
import com.github.h0tk3y.betterParse.parser.Parser
import ae.hitb.proctf.drone_racing.programming.languageUtils.resolveCalls
import ae.hitb.proctf.drone_racing.programming.language.*

object ProgramGrammar : Grammar<Program>() {

    private val LPAR by token("\\(")
    private val RPAR by token("\\)")

    private val PLUS by token("\\+")
    private val MINUS by token("-")
    private val DIV by token("/")
    private val MOD by token("%")
    private val TIMES by token("\\*")
    private val OR by token("\\|\\|")
    private val AND by token("&&")
    private val EQU by token("==")
    private val NEQ by token("!=")
    private val LEQ by token("<=")
    private val GEQ by token(">=")
    private val LT by token("<")
    private val GT by token(">")

    private val NOT by token("!")

    private val COLON by token(":")
    private val COMMA by token(",")
    private val SEMICOLON by token(";")
    private val ASSIGN by token("=")
    private val DOTS by token("\\.\\.")

    private val DOLLAR by token("\\$")

    private val IF by token("if\\b")
    private val THEN by token("then\\b")
    private val ELIF by token("elif\\b")
    private val ELSE by token("else\\b")
    private val FI by token("fi\\b")

    private val WHILE by token("while\\b")
    private val FOR by token("for\\b")
    private val DO by token("do\\b")

    private val PASS by token("pass\\b")

    private val REPEAT by token("repeat\\b")
    private val UNTIL by token("until\\b")

    private val BEGIN by token("begin\\b")
    private val END by token("end\\b")

    private val FUN by token("fun\\b")
    private val RETURN by token("return\\b")

    private val TRUE by token("True\\b")
    private val FALSE by token("False\\b")

    private val INTEGER_TYPE by token("int\\b")
    private val STRING_TYPE by token("str\\b")

    private val NUMBER_LITERAL by token("\\d+")
    private val CHAR_LITERAL by token("'.'")
    private val STRING_LITERAL by token("\".*?\"")

    private val ID by token("[A-Za-z_]\\w*")

    private val WS by token("\\s+", ignore = true)
    private val NEWLINE by token("[\r\n]+", ignore = true)

    private val signToKind = mapOf(
            OR to Or,
            AND to And,
            LT to Lt,
            GT to Gt,
            EQU to Eq,
            NEQ to Neq,
            LEQ to Leq,
            GEQ to Geq,
            PLUS to Plus,
            MINUS to Minus,
            TIMES to Times,
            DIV to Div,
            MOD to Rem
    )

    private val intLiteral by
    (optional(MINUS) map { if (it == null) 1 else -1 }) * NUMBER_LITERAL map { (s, it) -> IntLiteral(s * it.text.toInt()) }

    private val intConst by
    intLiteral or
            CHAR_LITERAL.map { IntLiteral(it.text[1].toInt()) } or
            (TRUE asJust IntLiteral(1)) or
            (FALSE asJust IntLiteral(0))

    private val functionCall: Parser<FunctionCall> by
            (ID * -LPAR * separatedTerms(parser(this::expression), COMMA, acceptZero = true) * -RPAR).map { (name, args) ->
                FunctionCall(UnresolvedFunction(name.text, args.size), args)
            }

    private val variable by ID use { Variable(text) }

    private val param by -DOLLAR * ID use { Param(text) }

    private val stringLiteral by STRING_LITERAL use { StringLiteral(text.removeSurrounding("\"", "\"")) }

    private val notTerm by (-NOT * parser(this::term)) map { UnaryOperation(it, Not) }
    private val parenTerm by -LPAR * parser(this::expression) * -RPAR

    private val term: Parser<Expression> by
        intConst or functionCall or notTerm or variable or param or parenTerm or stringLiteral

    private val multiplicationOperator by TIMES or DIV or MOD
    private val multiplicationOrTerm by leftAssociative(term, multiplicationOperator) { l, o, r ->
        BinaryOperation(l, r, signToKind[o.type]!!)
    }

    private val sumOperator by PLUS or MINUS
    private val mathExpression: Parser<Expression> = leftAssociative(multiplicationOrTerm, sumOperator) { l, o, r ->
        BinaryOperation(l, r, signToKind[o.type]!!)
    }

    private val comparisonOperator by EQU or NEQ or LT or GT or LEQ or GEQ
    private val comparisonOrMath: Parser<Expression> by (mathExpression * optional(comparisonOperator * mathExpression))
            .map { (left, tail) -> tail?.let { (op, r) -> BinaryOperation(left, r, signToKind[op.type]!!) } ?: left }

    private val andChain by leftAssociative(comparisonOrMath, AND) { l, _, r -> BinaryOperation(l, r, And) }
    private val orChain by leftAssociative(andChain, OR) { l, _, r -> BinaryOperation(l, r, Or) }
    private val expression: Parser<Expression> by orChain

    private val passStatement: Parser<Pass> by PASS.map { Pass }

    private val functionCallStatement: Parser<FunctionCallStatement> by functionCall.map { FunctionCallStatement(it) }

    private val assignStatement: Parser<AssignStatement> by
        (variable * -ASSIGN * expression).map { (v, e) -> AssignStatement(v, e) }

    private val ifStatement: Parser<IfStatement> by
            (-IF * expression * -THEN *
             parser { statementsChain } *
             zeroOrMore(-ELIF * expression * -THEN * parser { statementsChain }) *
             optional(-ELSE * parser { statementsChain }).map { it ?: Pass } *
             -END
            ).map { (condExpr, thenBody, elifs, elseBody) ->
                val elses = elifs.foldRight(elseBody) { (elifC, elifB), el -> IfStatement(elifC, elifB, el) }
                IfStatement(condExpr, thenBody, elses)
            }

    private val forStatement: Parser<ChainStatement> by
            (-FOR * parser { variable } * -ASSIGN * parser { expression } * -DOTS * parser { expression } * -DO * -BEGIN *
                    parser { statementsChain } * -END
                    ).map { (variable, fromExpression, toExpression, body) ->
                ChainStatement(
                    AssignStatement(variable, fromExpression),
                    WhileStatement(
                        BinaryOperation(variable, toExpression, Leq),
                        ChainStatement(body, AssignStatement(variable, BinaryOperation(variable, IntLiteral(1), Plus)))
                    )
                )
            }

    private val whileStatement: Parser<WhileStatement> by (-WHILE * expression * -DO * -BEGIN * parser { statementsChain } * -END)
            .map { (cond, body) -> WhileStatement(cond, body) }

    private val repeatStatement: Parser<ChainStatement> by (-REPEAT * parser { statementsChain } * -UNTIL * expression).map { (body, cond) ->
        ChainStatement(body, WhileStatement(UnaryOperation(cond, Not), body))
    }

    private val returnStatement: Parser<ReturnStatement> by -RETURN * expression map { ReturnStatement(it) }

    private val statement: Parser<Statement> by passStatement or
            functionCallStatement or
            assignStatement or
            ifStatement or
            whileStatement or
            forStatement or
            repeatStatement or
            returnStatement

    private val typeName: Parser<FunctionType> by (INTEGER_TYPE map { FunctionType.INTEGER }) or (STRING_TYPE map { FunctionType.STRING })

    private val functionArgument: Parser<Pair<Variable, FunctionType>> by
            ID * -COLON * typeName map { (name, type) -> Pair(Variable(name.text), type)}

    private val functionType: Parser<FunctionType> by
            -COLON * typeName

    private val functionDeclaration: Parser<FunctionDeclaration> by
            (-FUN * ID * -LPAR * separatedTerms(functionArgument, COMMA, acceptZero = true) * -RPAR * optional(functionType) * -BEGIN * parser { optional(statementsChain) } * -END)
                    .map { (name, parameters, returnType, body) ->
                        FunctionDeclaration(name.text, parameters.map { it.first }, parameters.map { it.second }, returnType ?: FunctionType.VOID, body ?: Pass)
                    }

    private val statementsChain: Parser<Statement> by
            separatedTerms(statement, optional(SEMICOLON)) * -optional(SEMICOLON) map { chainOf(*it.toTypedArray()) }

    override val rootParser: Parser<Program> by oneOrMore(functionDeclaration or (statement and optional(SEMICOLON) use { t1 })).map {
        val functions = it.filterIsInstance<FunctionDeclaration>()
        val statements = it.filterIsInstance<Statement>().let { if (it.isEmpty()) listOf(Pass) else it }
        val rootFunc = FunctionDeclaration("main", listOf(), listOf(), FunctionType.VOID, chainOf(*statements.toTypedArray()))
        Program(functions + rootFunc, rootFunc)
    }
}

internal fun readProgram(text: String): Program {
    val parsed = ProgramGrammar.parseToEnd(text)
    return resolveCalls(parsed)
}