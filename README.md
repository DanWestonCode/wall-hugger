# Wall Hug Actor Component

A prototype for a wall hugging game mechanic on a 3rd person character controller. This has been implemented as a UActorComponent. The wall hug works by the player "pushing" the character into a wall and holding directional input (Metal Gear Solid style), as opposed to pressing a button to start and end the sequence. 

## Video:
<a href="http://www.youtube.com/watch?feature=player_embedded&v=7Nqz06_JiOI
" target="_blank"><img src="http://img.youtube.com/vi/7Nqz06_JiOI/0.jpg" 
alt="Wall Hugging Demo" width="480" height="360" border="10" /></a>

## Summary

The guts of this mechanic lives inside the `UActorComponent`, `PWallHugComponent.h/cpp`.

To start the wall hug the player must approach a wall and "push" into it:
- Detect collision with obstacle using `OnComponentHit`.
- Determine if Character is facing the wall within the given tolerance - `PWallHugComponent::ApproachingWallTolerance`

<div align="center">
  <figure style="display: inline-block; margin: 10px;">
    <img src="ReadmeContent/inside.png" width="300" />
    <figcaption><b>Inside ApproachingWallTolerance</b></figcaption>
  </figure>
  <figure style="display: inline-block; margin: 10px;">
    <img src="ReadmeContent/outside.png" width="300" />
    <figcaption><b>Outside of ApproachingWallTolerance</b></figcaption>
  </figure>
</div>

- Using the collision normal, create a rotation for the character to face away from the colliding obstacle.
- Calculate the perpendicular axis for "sliding" the character along the surface (`UWHWallHugComponent::ProcessBeginWallHug`).

Once the player is on the wall they must hold the directional input they used to start the wall hug and then use the perpendicular axis to slide the character along the wall. `UWHWallHugComponent::ProcessMovementInput` processes all the input handling whilst the player is on the wall:
- Determine if the player is still holding the starting input direction against the wall.
- Detect whether the player is pointing enough towards the current `UWHWallHugComponent::SlideAxis`.
- Inform the character of any movement (`APawn::AddMovementInput`) according to the above. 

During the wall hug, a capsule sweep is performed behind the player to ensure they are still against an obstacle (`UWHWallHugComponent::ProcessOnWallState`). Several variables are also recalculated including: 
- The target rotation for the character for keeping their back to the surface.
- The perpendicular of the surface being hugged. This is used for sliding the player correctly across the surface. 

`UWHWallHugComponent` sets a publicly accessible state, `EWallHugState`, which can be accessed by C++ and Blueprints. `UWHWallHugComponent` utilises the current state to switch the current processing logic it needs to run for the mechanic. The animation system utilises the component `EWallHugState` for driving the current locomotion state logic. 

The animations are from Mixamo (see below for references or just grab from the project). They are retargeted to Unreal Quinn using the Retarget Animations tool (right click on src animation and search for  “Retarget Animations”).
<p>
    <img src="ReadmeContent/animation_retarget.png" width="75%" />
</p>

A 2D Blendspace is used for the slide animation. The x-axis controls direction whilst the y-axis controls the speed. The animations are blended between Left cover sneak, Leaning and Right cover sneak.
<p>
    <img src="ReadmeContent/blendspace.png" width="50%" />
</p>

The animation and locomotion is controlled via the characters Animation Blueprint. The Animation Blueprint uses the Wall Hug Component for understanding the current state of the wall hug (`UWHWallHugComponent::GetState`). 
<p>
   <figure style="display: inline-block; margin: 10px;">
    <img src="ReadmeContent/abp_eventgraph.png"/>
    <figcaption><b>Acquiring Wall Hug Component</b></figcaption>
  </figure>
</p>

The characters Animation Blueprint has been adapted for an additional State and Pose for the Wallhug Locomotion: 
<p>
    <img src="ReadmeContent/animgraph_pose.png" width="50%" />
</p>
The wall hug locomotion is a Blendspace player, utilising the 2D Blendpsace above: 
<p>
    <img src="ReadmeContent/wallhug_locomotion.png" width="75%" />
</p>
The Ground Speed comes from the Character Movement component (setup in ABP_Unaramed by default), and the Wall Hug Direction is calculated from the Actors right vector and current velocity within the Event Graph.

<p>
   <figure style="display: inline-block; margin: 10px;">
    <img src="ReadmeContent/calculating_wallhug_direction.png"/>
    <figcaption><b>Calculating wall hug direction</b></figcaption>
  </figure>
</p>

The Wallhug Locomotion pose is plugged into the rest of the AnimGraph.
<p>
    <img src="ReadmeContent/abp_graph.png"/>
</p>
The state transitions assess the Wall Hug Components current `EWallHugState`, moving into the Wall Hug locomotion when `state != ENone` and out when `state == ENone`.
<p>
    <img src="ReadmeContent/state_rule.png"/>
</p>

## Mixamo animations: 
- [Left cover sneak](https://www.mixamo.com/#/?page=1&query=Left+cover+sneak)
- [Right cover sneak](https://www.mixamo.com/#/?page=1&query=Right+cover+sneak)
- [Leaning](https://www.mixamo.com/#/?page=1&query=leaning)
## Remarks/TODOs:
- Explore the use of IK to correct animation and wall contact points. 
- Crouching wall hug state.
## Licence

Copyright (c) 2026 Daniel Weston

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
