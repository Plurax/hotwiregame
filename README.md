# The hot wire game

The hot buzzer wire game (German: Der heiße Draht)
The game ist built with 3 signals:

- green  - A: The "host of the buzzer slope
             Touching this with the buzzer slope will cause reset to 0 for the time
             Releasing it will start measurement - player moves onto B
- red    - B: This is the touch point on the other side of the board. Touching it confirms that the player moved
             on the complete wire.
- yellow - D: The wire. Touching this will increment the touch cont. Target ist to move the buzzer slope from A to
             B and back to A without touching D.

As the trigger inputs are low active, you can add this board to an existing circuit with 5V LEDs easily (The slope is GND)

# Sound

The sketch uses an active buzzer on A3.
The code currently plays a short beep if the player touches the wire.
If the player touches B and returns to A a short melody is played.

# Display

The sketch uses an OLED display with 128x32 pixels - the U8glib is needed to use it.

The display shows the time nad touchcount when playing and when touching A it shows the last time and touch count.
