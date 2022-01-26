# mms-dispenser
Arduino code for an M&amp;Ms dispenser

- The arduino will connect to WIFI and then fetch repeatedly data (JSON) through the API of Shortcut.com.
- The story points of the last completed story is retrieved for every colleague and the story id is cached (for comparison later on).
- The LCD will display the name of the colleague and the story points earned.
- The arduino will then play the colleagues preferred song through tne the piezo element (on the back).
- The arduino will then make the (continuous) servo turn for a certain time, which depends on the earned story points.

![mms-dispenser](https://user-images.githubusercontent.com/5860729/151228811-12450e71-abfd-4ec6-8e34-1062e9868183.jpeg)

