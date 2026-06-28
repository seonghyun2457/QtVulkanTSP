# Qt + Vulkan Pathfinder

An interactive grid pathfinding visualizer built with **Qt6** and rendered through a custom **Vulkan** backend as Grphic API. Draw walls, place a start and an end node, pick an algorithm, and watch the search explore the grid and reconstruct the shortest path in real time.

![Problem setup](images/ProblemExample.png)

## Features

- **Four pathfinding algorithms** — Breadth-First Search (BFS), Depth-First Search (DFS), A\*, and Dijkstra, selectable at runtime.
- **Vulkan-rendered grid** — every cell is drawn as a quad through a hand-rolled Vulkan renderer and swapchain, with live CPU FPS and GPU timing reported in the window title bar.
- **Gradient solution path** — once the goal is reached, the reconstructed shortest path is marked with a dedicated `SolutionNode` status and rendered as a smooth color gradient that fades from the ending node's color to the starting node's color along the route.
- **Interactive editing** — paint and erase nodes with the mouse to design arbitrary mazes and obstacle layouts.
- **Configurable problem size** — choose the grid dimensions up to **40 rows × 50 columns**.
- **Customizable node colors** — recolor any node type (start, end, wall, movable, visited) via a standard color picker.
- **Diagnostics tabs** — a *Vulkan Info* tab and a live *Debug Log* tab at the bottom of the window.

## How it works

The grid is a flat `std::vector<Node>`, each `Node` carrying an `eNodeStatus`
(`StartingNode`, `EndingNode`, `BlockingNode`, `MovableNode`, `VisitedNode`,
`SolutionNode`). `PathFinder::solve()` dispatches to the chosen `eSolver` and
returns both the solution indices and the list of visited indices, which the
renderer colors on the grid. The visited nodes are filled in cyan as the
frontier expands, and every node on the returned path is then promoted to
`SolutionNode` and tinted by linearly interpolating between the ending and
starting node colors — so the path reads as a gradient from one endpoint to the
other. Pressing **Reset** demotes those `SolutionNode` (and `VisitedNode`) cells
back to `MovableNode` while leaving your walls intact.

## Usage

### 1. Set the problem size

Use the **Problem size** drop-downs to choose the number of rows and columns. The grid resizes immediately to match.

![Smaller grid](images/ProblemSizeChanged.png)

### 2. Design the grid

Select a node type under **Node Colors**, then click and drag on the grid to paint:

- **Starting node** (green) — where the search begins.
- **Ending node** (red) — the target.
- **Blocking node** (white) — impassable walls.
- **Movable node** (black) — open, traversable cells.
- **Visited node** (cyan) — marked by the algorithm as it explores.

### 3. Pick an algorithm and solve

Choose **BFS**, **DFS**, **A\***, or **Dijkstra** under **Solving algorithms**, then press **Solve**. Visited cells fill in as the frontier expands, and once the goal is reached the final route is highlighted as a color gradient running from the ending node to the starting node.

| BFS | DFS |
| :---: | :---: |
| ![BFS result](images/BFSResult.png) | ![DFS result](images/DFSResult.png) |
| **A\*** | **Dijkstra** |
| ![A* result](images/AstarResult.png) | ![Dijkstra result](images/DijkstraResult.png) |

The **Reset** button clears the computed solution while keeping your walls, and **Clear** wipes the grid back to empty.

### 4. Recolor nodes (optional)

Click a node-color swatch to open the color picker and assign any color to a node type.

![Color picker](images/ColorSelection.png)

The new color is applied across the grid immediately.

![After recoloring the start node](images/AfterColorSelection.png)

## Build

Requirements:

- Qt 6.5+ with `Qt6::Core`, `Qt6::Widgets`, and `Qt6::Gui`
- CMake 3.19+
- A C++23-capable compiler (MinGW 64-bit on Windows)
- A Vulkan-capable GPU and driver
- [GLM](https://github.com/g-truc/glm) (vendored under `externals/glm`)

Open `CMakeLists.txt` in Qt Creator and build with the configured kit (Qt 6.10.2, MinGW 64-bit).

## Project layout

```
header/      all .h files (MainWindow, VulkanWidget, VulkanRenderer, Swapchain,
             PathFinder, Node, Rectangle, GroupSolver, GroupNodeColor, enums…)
src/         all .cpp files and Qt .ui forms
shaders/     Vulkan shader sources
externals/   vendored dependencies (GLM)
images/      screenshots used in this README
```

**Core types**

- `MainWindow` — hosts the UI, wires up signals, and owns the Vulkan widget.
- `VulkanWidget` / `VulkanRenderer` / `Swapchain` — the Vulkan rendering stack that draws the grid.
- `PathFinder` — static dispatcher implementing BFS, DFS, Dijkstra, and A\*.
- `Node` / `Rectangle` — a grid cell and its renderable quad.
- `GroupSolver` / `GroupNodeColor` — the right-hand control panels (algorithm selection, problem size, node colors, buttons).
