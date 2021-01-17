import { Box, SimpleGrid, Flex, Badge, Grid } from "@chakra-ui/react";
import { Container } from "../components/Container";
import { Dispatch, SetStateAction, useEffect, useState } from "react";
import { GameState } from "../model/GameState";
import { GameConfig } from "../model/GameConfig";
import { CellType } from "../model/CellType";
import { hostname } from "../global/variables";
import { Move } from "../model/Move";
import { Snake } from "../model/Snake";

const Index = () => {
  const [gameState, setGameState] = useState<GameState>();
  const [gameConfig, setGameConfig] = useState<GameConfig>();
  const [gameRows, setGameRows] = useState<CellType[][]>();

  useEffect(() => {
    fetch(hostname + "/game_config")
      .then((r) => r.json())
      .then((data) => {
        setGameConfig(data);
        setInterval(() => {
          updateGameState(data, setGameState, setGameRows);
        }, 1000);
      });

    document.addEventListener("keypress", (k) => {
      console.log(k.key);
      let move: Move;
      switch (k.key) {
        case "w":
          move = Move.Up;
          break;
        case "s":
          move = Move.Down;
          break;
        case "d":
          move = Move.Right;
          break;
        case "a":
          move = Move.Left;
      }
      console.log(move);
      if (move !== undefined) {
        fetch(hostname + "/move/" + move);
      }
    });
  }, []);

  return (
    <Container height="100vh">
      {gameConfig && gameRows ? (
        <Grid templateColumns="1fr 220px 650px 220px 1fr" mt={5}>
          <Box></Box>
          <Box></Box>
          <SimpleGrid columns={gameConfig.width} spacing={1} borderWidth={2}>
            {gameRows
              .map((gr, i) =>
                gr.map((cell, j) => {
                  let color = "transparent";
                  switch (cell) {
                    case CellType.Food:
                      color = "green.400";
                      break;
                    case CellType.SnakeBody:
                      color = "gray.300";
                      break;
                    case CellType.SnakeHead:
                      color = "gray.500";
                      break;
                    case CellType.MySnakeHead:
                      color = "orange.500";
                      break;
                    case CellType.MySnakeBody:
                      color = "orange.300";
                  }
                  return (
                    <Box bg={color} key={`${j}_${i}`} w="15px" h="15px"></Box>
                  );
                })
              )
              .flat()}
          </SimpleGrid>
          <Box ml={5} borderWidth={2} p="2">
            <Box fontWeight="bold" fontSize="lg" as="h4">
              Points
            </Box>
            {gameState.snakes
              .sort((s1, s2) => s2.points - s1.points)
              .map((snake, i) => (
                <Flex alignItems="center" justifyContent="space-between">
                  <Box fontWeight="semibold">
                    Snake {i + 1}
                  </Box>
                  <Flex alignItems="center">
                    <Box fontWeight="semibold">
                      {snake.points}
                    </Box>
                    <Box color="gray.500" ml="5px">
                      Points
                    </Box>                    
                    {snake.is_me ? (
                      <Badge borderRadius="full" ml="4px" colorScheme="teal" w="25px">
                        Me
                      </Badge>
                    ) : <Box w="25px" ml="4px"></Box>}
                  </Flex>
                </Flex>
              ))}
          </Box>
        </Grid>
      ) : (
        <Box>Loading...</Box>
      )}
    </Container>
  );
};

function updateGameState(
  gameConfig: GameConfig,
  setGameState: Dispatch<SetStateAction<GameState>>,
  setGameRows: Dispatch<SetStateAction<CellType[][]>>
) {
  if (!gameConfig) {
    return;
  }
  fetch(hostname + "/game_state")
    .then((r) => r.json())
    .then((data) => {
      setGameState(data);
      let cellTypes: CellType[][] = [];
      for (let i = 0; i < gameConfig.height; i++) {
        cellTypes[i] = new Array(gameConfig.width).fill(CellType.Empty);
      }

      data.foods.forEach((food) => {
        cellTypes[food[1]][food[0]] = CellType.Food;
      });
      data.snakes.forEach((snake: Snake) => {
        snake.nodes.forEach((node, i) => {
          if (i == 0) {
            cellTypes[node[1]][node[0]] = snake.is_me
              ? CellType.MySnakeHead
              : CellType.SnakeHead;
          } else if (
            cellTypes[node[1]][node[0]] === CellType.Food ||
            cellTypes[node[1]][node[0]] == CellType.Empty
          ) {
            cellTypes[node[1]][node[0]] = snake.is_me
              ? CellType.MySnakeBody
              : CellType.SnakeBody;
          }
        });
      });
      setGameRows(cellTypes);
    });
}

export default Index;
