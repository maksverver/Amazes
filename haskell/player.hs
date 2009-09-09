import System.Exit
import Array
import List
import Maybe
import IO

just :: Maybe a -> a
just (Just x) = x

data Dir        = North | East | South | West deriving (Enum, Show)
data RelDir     = Forward | ToRight | Backward | ToLeft deriving (Enum, Show)
type Point      = (Int,Int)

data Square = Square { discovered :: Bool,
                       wallNorth  :: Maybe Bool,
                       wallEast   :: Maybe Bool,
                       wallSouth  :: Maybe Bool,
                       wallWest   :: Maybe Bool } deriving Show

type Maze = Array Point Square

data GameState  = GameState { maze      :: Maze,
                              location  :: Point,
                              direction :: Dir } deriving Show

type ViewInfo   = [ (Dir, String) ]
type TurnInfo   = (ViewInfo, Int)

type Turn = String

turnDir :: Dir -> RelDir -> Dir
turnDir d r = toEnum (mod (fromEnum d + fromEnum r)  4)

inferMaze :: Maze -> Maze
inferMaze maze = maze  -- TODO!

doMove :: GameState -> Char -> GameState
doMove state dir = state  -- TODO!

updateView :: GameState -> ViewInfo -> GameState
updateView state lines = foldl update state lines
    where update state (relDir, line) = state  -- TODO!

pickTurn :: (GameState, Turn) -> TurnInfo -> (GameState, Turn)
pickTurn (lastState, lastTurn) (view, distSq)
    = (state, "T")
    where state = updateView (foldl doMove lastState lastTurn) view

initialGameState :: GameState
initialGameState = GameState initialMaze (0,0) North
    where   initialMaze = array ((0,0),(0,0)) [ ((0,0), initialSquare) ]
            initialSquare = Square True Nothing Nothing Nothing Nothing

main :: IO (GameState, Turn)
main = foldl (>>=) (return (initialGameState, "")) (replicate 2 doTurn)
    where

    readLines :: Int -> IO [String]
    readLines 0 = return []
    readLines n = hGetLine stdin >>= handleLine
        where
            handleLine :: String -> IO [String]
            handleLine "Start" = readLines n
            handleLine "Quit"  = exitSuccess
            handleLine line    = do lines <- readLines (n-1)
                                    return (line:lines)

    parseTurnInfo :: [String] -> TurnInfo
    parseTurnInfo [n,e,s,w,d]
        = ([(North, n), (East, e), (South, s), (West, w)], read d)

    doTurn :: (GameState, Turn) -> IO (GameState, Turn)
    doTurn prev = do
        [n,e,s,w,d] <- readLines 5
        let next = pickTurn prev ([(North, n), (East, e),
                   (South, s), (West, w)], read d) in do
        hPutStrLn stdout (snd next)
        hFlush stdout
        return next
