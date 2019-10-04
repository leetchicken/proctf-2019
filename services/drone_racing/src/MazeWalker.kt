package ae.hitb.proctf.drone_racing

class StaticMazeWalker {
    companion object {
        // TODO: удалять старые
        private val states: MutableMap<Thread, MazeWalker> = mutableMapOf()

        @JvmStatic
        fun goRight() = states[Thread.currentThread()]!!.goRight()

        @JvmStatic
        fun goLeft() = states[Thread.currentThread()]!!.goLeft()

        @JvmStatic
        fun goUp() = states[Thread.currentThread()]!!.goUp()

        @JvmStatic
        fun goDown() = states[Thread.currentThread()]!!.goDown()

        @JvmStatic
        fun setMaze(mazeString: String) {
            // TODO сделать лабиринты только квадартными, чтобы можно было звать без дополнительных аргументов
            states[Thread.currentThread()] = MazeWalker(Maze(mazeString))
        }

        @JvmStatic
        fun getMovesCount() = states[Thread.currentThread()]!!.movesCount
    }
}

class MazeWalker(private val maze: Maze, var x: Int = 0, var y: Int = 0, var movesCount: Int = 0) {
    fun goUp() {
        if (y == maze.height - 1)
            throw BadMove("Can't go up!")
        if (maze.map[x][y + 1])
            throw BadMove("Can't go up: it's wall there!")
        y += 1
        movesCount += 1
    }

    fun goDown() {
        if (y == 0)
            throw BadMove("Can't go down!")
        if (maze.map[x][y - 1])
            throw BadMove("Can't go down: it's wall there!")
        y -= 1
        movesCount += 1
    }

    fun goLeft() {
        if (x == 0)
            throw BadMove("Can't go left!")
        if (maze.map[x - 1][y])
            throw BadMove("Can't go left: it's wall there!")
        x -= 1
        movesCount += 1
    }

    fun goRight() {
        if (x == maze.width - 1)
            throw BadMove("Can't go right!")
        if (maze.map[x + 1][y])
            throw BadMove("Can't go right: it's wall there!")
        x += 1
        movesCount += 1
    }
}

class BadMove(message: String) : Throwable(message)