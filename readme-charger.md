# <a name='pogobot-charger'></a>Pogobot Charger

There is two ways to recharge a pogobot:
- by the programming cable
- by the plus and minus pads on the head

In order to charge multiple robot at once, we develop a charger using pogopins that connect the plus and minus on the head of the robot by flipping the robot.

<img src="Images/charger.jpg" alt="pogobot charger" width="800"/>

The PCB cad files are available in the folder.
The pogopin used are these : https://www.digikey.fr/en/products/detail/preci-dip/90224-AS/5451911
We also used one power jack 5.1 embase connected to the power outlet. The second one is used, if you want to daisy chain multiple PCB (be careful, the total amps used).
:warning: The power plug has to expose the +5v on the inner contact and Ground on the outer contact. Please verify with a voltmeter before pluggin a robot.

We developed some small feet to mount with M3 screws on the PCB and some small cones in order to help the contacts for each robot.
Both 3d designs are available in the folder "3d-addons" (pogocharger*).