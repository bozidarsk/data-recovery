# Data Recovery Game

A Sofia University course project.

## How to Build

- using gcc (with `g++ main.cpp -o data-recovery` or `gcc main.cpp -o data-recovery -lstdc++`)
- using cmake (can generate visual studio build files)

## How to Play

### Starting the Game

When creating a new game, you will be prompted to enter:

1. **A file path** – absolute or relative path to a file containing the *original* text. The file itself is never modified.
2. **Corruption ratio** – a number between **0.0** and **1.0**, indicating what percentage of the text will be corrupted.

Example input:
```
path/to/source/file.txt
0.5
```

After reading the input, the game displays a corrupted version of the text.

### Inspecting and Recovering Text

After the corrupted text is shown:
1. Enter the **index of the word** you want to inspect.
2. Enter the **index of the character** within that word.

The game will then display **all 6 possible variants** of the selected character.

If you select a character:
- The corrupted text is updated.
- If the choice matches the original character, it is colored **green**.
- If the choice does not match the original character, it is colored **red**, and the move counts as a **mistake**.

### Winning the Game

The game ends when the corrupted text is fully restored to its original state. You will see the total number of mistakes you made.
