# v0.4.4-prediction-test - Experimental shot prediction

This local test build addresses three findings from the native code review:
the game can refresh range and mode after the existing hold-fire hook, it can
blend the previous and current aim transforms before emission, and a sphere
capsule was missed by the geometry solver. The new gate checks both possible
aim modes and conservatively covers the transform blend. Startup path lengths
are checked before constructing adjacent INI and log names.

The existing hook cannot know the prepared shot's final target distance. The
conservative check can hold fire when the player is behind the intended target.
It can also add aim work on Neutral/Friend shots. This exact build requires
in-game behavior and performance testing before publication. Long pauses
remain unresolved.

The exact prior 0.4.3 ASI was validated in separate hostile, friendly and
neutral runs. That evidence does not establish the behavior of this build.
