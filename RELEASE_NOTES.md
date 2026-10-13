# v0.4.4-prediction-test - Experimental shot prediction

This local test build addresses three findings from the native code review:
the game can refresh range and mode after the existing hold-fire hook, it can
blend the previous and current aim transforms before emission, and a sphere
capsule was missed by the geometry solver. The new gate checks both possible
aim modes and conservatively covers the transform blend. Startup path lengths
are checked before constructing adjacent INI and log names.

The existing hook cannot know the prepared shot's final target distance. The
conservative check can hold fire when the player is behind the intended target.
It can also add aim work on Neutral/Friend shots. One mixed in-game run showed
191 Neutral checks with 108 shots withheld, 1,103 Friend checks with 540 withheld,
and 221 hostile checks with none withheld. The tester reported normal behavior.
The longest hook call was 10.7 microseconds, with no guarded-read faults or
dropped frame samples. This was not a matched FPS comparison. Long pauses
remain unresolved; long frame gaps in this capture contained no hook calls.

The exact prior 0.4.3 ASI was validated in separate hostile, friendly and
neutral runs. That evidence does not establish the behavior of this build.
