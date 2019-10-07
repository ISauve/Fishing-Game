# Fishing Game

![](Assets/preview.gif)

### CS 488 Final Project, Spring 2019

The purpose of this project was to learn more about OpenGL and various 3D rendering techniques through the implementation of a simple game. Through this project, I was able to explore the following topics:  
• Rendering fluid  
• Rendering a sky which shows the passage of time  
• Rendering shadows & lens flare  
• Implementing collision detection between 3D objects, both static and dynamic  

The scene rendered is a small lake set in a hilly outdoor landscape, with trees and rocks decorating the terrain. On this lake is a small boat which can be moved using the W/A/S keys; the boat can be viewed in either third or first person view, and in both viewing modes the player can look around the scene by dragging the mouse horizontally (to look left/right) or vertically (to look up/down). In the lake, there are 10 fish whose initial location, direction of movement and movement speed is randomly determined at initialization (within reasonable ranges). The fish swim in the direction they are facing and every frame they have a 2% chance of changing directions by rotating in the Y axis anywhere from -30 to 30 degrees. When a fish collides with another fish or with the terrain, it simply turns around and continues moving in the opposite direction. The objective of the game is to find and catch all 10 fish by driving the boat overtop of them - when a collision is detected between a fish and the boat, that fish is ”caught”, causing the fish disappear and a counter in the upper right hand corner of the screen to increase. When the player has caught all 10 fish, they can reset the game by pressing the R button and play again.

In order to make the scene more visually appealing, a dynamic skybox is rendered which rotates at a speed which can be controlled by the player through the Information widget (accessible by pressing the I key). After the skybox completes a full rotation, it smoothly transitions from day into night, and then after another full rotation it transitions back. As well, during the day the sun can be seen rising in the east, reaching its zenith halfway through the day, and then setting in the west. The sun acts as the sole light source of the scene, and thus as time passes the lighting of the scene changes in response to its movement; as well, shadow mapping is used to make the objects in the scene cast shadows onto the terrain, and as the sun moves the shadows also move in response to its movement. Finally, the sun can be looked at directly when the player is in first person mode, and doing so results in a lens flare effect being rendered to the screen, with the intensity and position of the effect varying as a factor of the distance from the sun to the center of the screen.

## Compilation

To compile the program, [Open Asset Import Library (Assimp)](https://github.com/assimp/assimp) must be properly installed so that the compiler can find the appropriate headers using calls such as `#include <assimp/scene.h>`.

If this requirement is met, you can compile & run the program using the following commands:

```
premake4 gmake  
make  
./FishingGame  
```

> Note: only macOS & linux are supported

## Playing the game

The objective of the game is to catch all the fish in the lake by "driving" over them with your boat.  The GUI indicator in the upper right hand of the screen indicates how many
of the fish you have caught, out of a total of 10.

User interaction:  
- W: Moves the boat forward. If the water gets too shallow in the direction you move, your speed will slow down (to a complete stop at a certain depth, at which point you must turn around
 to keep moving).  
- A: Turns the boat left   
- S: Turns the boat right  
- Q: Quits the game  
- R: Resets the game  
- I: Toggles the information widget, which can be used to modify the rendering settings (for example, to toggle between first & third person view)  
- Mouse drag in the X direction: Rotates the view around the boat  
- Mouse drag in the Y direction: Changes the pitch of the view  
- Mouse scroll: Changes the zoom level (third person mode only)  
