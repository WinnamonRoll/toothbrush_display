# toothbrush showcase box

## background
I was contacted by a friend of my friend to help her design and build a simple interactive toothbrush showcase box for a dental care company. Although this project was supposed to be a simple small project, I was hesitant to accept the project as I am aware that I am an electronics noob. However, the hobbyist side of me told me to accept the project, as this project will be a great opportunity for me to revive my knowledge in mechatronics and embedded systems programming, which, admittedly, have become quite rusty since I graduated. Besides, I got paid for all the components.

As a person who has worked with similar projects before, my first instinct is to use an Arduino to control the showcase box. However, what is particularly challenging for me in this project is the circuitry. Previously, I used to make a circuit with my own hands, either by using jumper wires on a breadboard or putting each component on a prototype board and soldering them one-by-one. Needless to say that was very time consuming and also prone to errors. I remember that there were a couple of times in my university years that I had to pull an all-nighter to solder prototype circuit boards for class projects, and those were painful experiences. So, when I accepted this project, I told myself that I would not make a circuit board by hand anymore, but rather I would use EasyEDA to design my first PCB and also get it printed and shipped to me. Fortunately, the learning curve in designing a PCB is not as steep as I thought (at least for a simple circuit board like this) as there are plenty of useful learning resources on the internet. I was very excited when the parcel arrived at my house, and I think I did an okay job for my first PCB. Nevertheless, I still made some design flaws which I am working on the second version to correct them.

<p align="center">
  <img width="460" height="300" src="/assets/images/pcb.jpg">
</p>

## project description
The toothbrushes will be moved back and forth using lead screw drives as customers interact with the box via buttons. Also, the customers can try using the toothbrush to brush the teeth model where the brushing weight will be monitored and shown on a mini OLED screen. This showcase box will allow customers to get a sense of how heavy they regularly brush their teeth so that they can choose their next toothbrush appropriately.

<p align="center">
  <img width="460" height="300" src="/assets/images/showcase.gif">
</p>

## known design mistakes
- Originally, I designed the PCB to use with an Arduino Nano v3.0; however, I made a mistake: I assumed that the pin A4 and A5 of an Arduino Nano can also be used as digital pins. I circumvented this mistake by opting for an Arduino Nano Every, where both pin A4 and A5 can be used as a digital pin. Nevertheless, Arduino Nano Every is more expensive than an Arduino Nano clone, so in the second version I would like to correct this mistake.
- Some female JST XH2.54 headers on the PCB are not correctly placed. I have to flip the pins in the male headers to correct this mistake, but as a perfectionist I don't like this at all. It makes me upset.

## known issues
- The function void display_weight_in_grams(Adafruit_SSD1306, int, int), which is supposed to nicely format the brushing weight and display it on the Adafruit OLED screen, is bugged. The formatting works fine but sometimes there can be weird characters overlaid on the screen. Currently, I have no idea what causes this, so I think I will ask about it on the Arduino forum. If you know what causes this, please let me know.
- I noticed that sometimes when I move the box around in my room, the box will suddenly stop working: The Arduino, stepper motors, and buttons are still powered, but the OLED screen will be off and the lead screw will not move when I press a button. I suspect that this is due to the wiring of the OLED screen being loose, causing the screen to be disconnected and freeze the whole program loop. Turning the power off and on again will fix this issue.
