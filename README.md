<h2 align="center">Prescient</h2>

<p align="center">
Open-source VST plugin - vocoder with pitch shifting.
</p>
<div align="center">
<img src="https://github.com/user-attachments/assets/63e10b8b-03e7-40e6-b980-2bd7ab21e0dc" width="450">
</div>

## Parameters
- LPC: vocoder on/off
- Order: vocoder order - has effect only with vocoder on
- Mono / stereo: choose stereo, mono, or anything in-between
- Dry / wet: ratio of effect signal to input signal
- Voice 1, 2, 3: advanced pitch shifting - first pitch shifts the voice, second and third add additional shifted copies

## Installation
For technical reasons, the effect is only compatible and verified to work with the following systems:
| OS  | DAW |
| ------------- | ------------- |
| Windows x64  | FL Studio, Ableton  |

The effect will not be ported to other systems nor ensured to work with other DAWS.

### Sidechain wiring
A carrier signal, e.g. guitar or synthesizer, is fed into one channel (green). The adjacent channel (blue) receives a voice signal - voice recording or microphone input. The effect is inserted on the carrier channel. To wire both signals into the effect, sidechain is used (connection highlighted in blue).

<div align="center">
<img src="https://github.com/user-attachments/assets/ea5cfbe6-e20d-4c62-ac29-b9cabf7fbaf9" width="260">
</div>


### Try it out
1. Download the .VST file
2. Unzip the file and place it in a VST directory like `C:\Program Files\Common Files\VST3\`
3. Follow the steps for <a href="#FL Studio">FL Studio</a> or <a href="#Ableton">Ableton</a>

---
### FL Studio

Click the insert effect button of the channel that you have selected for the carrier signal.

*More plugins* → *Manage plugins* → *Find more plugins*.

There, find the *Prescient-VST* plugin.
Star it and it will appear in the effect category *Fx-Vocals*.
Now, you can add it to the channel.

<div align="center">
<img src="https://github.com/user-attachments/assets/46c15971-b79a-4d9d-ad87-2cd3d5c1533d" width="650">
</div>

<div align="center">
<img src="https://github.com/user-attachments/assets/9b3b31f8-5c1c-4292-b219-36d8f926474d" width="540">
</div>

Prepare a channel with voice signal and sidechain it to the channel with effect by right-clicking on the triangle and *Sidechain to this track only*.

<div align="center">
<img src="https://github.com/user-attachments/assets/22b06cfd-9db7-4dab-b990-d25fe3a28610" width="320">
</div>

Ensure that the channels have the correct signals.

<div align="center">
<img src="https://github.com/user-attachments/assets/54323844-6e38-4e8e-9465-7e1e2f72d8af" width="320">
</div>

When you hit play, you should hear only the carrier signal. Set the plug-in to receive the sidechain signal:

<div align="center">
<img src="https://github.com/user-attachments/assets/66d1e575-cea7-44a3-9797-2d4730491d7e" width="320">
</div>

I recommend setting the plug-in window as detached.

---
### Ableton
*Settings* → *Plug-Ins* → *enable VST3*, *Rescan*.

In plugin browser (not pictured), find *Prescient-VST* under *BLCK*. Drag the effect onto the carrier track in *Arrangement View*.

<div align="center">
<img src="https://github.com/user-attachments/assets/818606b5-6ae3-4a7c-9aa7-aac4e0ff10a8" width="320">
</div>

Notice the carrier and voice tracks in *Arrangement View*. By clicking into the square with the name, open a list of effects. Assign the effect to the carrier track.

<div align="center">
<img src="https://github.com/user-attachments/assets/6ed81bb0-d583-4182-91a2-1a03f7a18776" width="500">
</div>

The sidechain input of the effect must be set so the effect can receive the signal from the voice track:

<div align="center">
<img src="https://github.com/user-attachments/assets/92a221d6-c30d-4dca-8554-0a951ad68452" width="360">
</div>

## Project description

</p>
<div align="center">
<img src="https://github.com/user-attachments/assets/8e81d350-7854-4392-8c73-fe33359a1b8e" width="450">
</div>

<div align="center">
<img src="https://github.com/user-attachments/assets/08df12e5-d2cc-4a10-9587-f09f3400d01a" width="450">
</div>


