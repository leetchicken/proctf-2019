package ae.hitb.proctf.drone_racing

import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.ConcurrentMap

open class StaticSharedMazeWalker {
    companion object {
        // TODO: удалять старые
        private val states: ConcurrentMap<Thread, MazeWalker> = ConcurrentHashMap<Thread, MazeWalker>()

        @JvmStatic
        fun right() = states[Thread.currentThread()]!!.goRight()

        @JvmStatic
        fun left() = states[Thread.currentThread()]!!.goLeft()

        @JvmStatic
        fun up() = states[Thread.currentThread()]!!.goUp()

        @JvmStatic
        fun down() = states[Thread.currentThread()]!!.goDown()

        @JvmStatic
        fun write(s: String) = states[Thread.currentThread()]!!.write(s)

        @JvmStatic
        fun isOnTopRightCell() = states[Thread.currentThread()]!!.isOnTopRightCell()

        @JvmStatic
        fun setMaze(mazeString: String) {
            states[Thread.currentThread()] = MazeWalker(Maze(mazeString))
        }

        @JvmStatic
        fun getMovesCount() = states[Thread.currentThread()]!!.movesCount

        @JvmStatic
        fun getOutput() = states[Thread.currentThread()]!!.output
    }
}

class MazeWalker(private val maze: Maze, var x: Int = 0, var y: Int = 0, var movesCount: Int = 0, var output: String = "") {
    fun goUp() {
        if (x == 0)
            throw BadMove("Can't go left!")
        if (maze.map[x - 1][y])
            throw BadMove("Can't go left: it's wall there!")
        x -= 1
        movesCount += 1
    }

    fun goDown() {
        if (x == maze.size - 1)
            throw BadMove("Can't go right!")
        if (maze.map[x + 1][y])
            throw BadMove("Can't go right: it's wall there!")
        x += 1
        movesCount += 1
    }

    fun goLeft() {
        if (y == 0)
            throw BadMove("Can't go down!")
        if (maze.map[x][y - 1])
            throw BadMove("Can't go down: it's wall there!")
        y -= 1
        movesCount += 1
    }

    fun goRight() {
        if (y == maze.size - 1)
            throw BadMove("Can't go up!")
        if (maze.map[x][y + 1])
            throw BadMove("Can't go up: it's wall there!")
        y += 1
        movesCount += 1
    }

    fun isOnTopRightCell() = (x == maze.size - 1) && (y == maze.size - 1)

    fun write(s: String) {
        output += s + "\n"
    }
}

class BadMove(message: String) : Throwable(message)