import { Snake } from "./Snake";

export interface GameState {
    snakes: Snake[];
    foods: number[][];
  }