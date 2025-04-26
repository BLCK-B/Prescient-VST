<template>
  <div class="app">
    <div class="knobs">
      <MyKnob class="knob" :default-val="100" knobText="Order" backendId="modelOrder" />
      <MyKnob class="knob" :default-val="50" knobText="Mono / stereo" backendId="monostereo" />
      <MyKnob class="knob" :default-val="100" knobText="Dry / wet" backendId="passthrough" />
      <MyKnobSplit class="knob" :default-val="0" knobText="Voice 1" backendId="shiftVoice1" />
      <MyKnobSplit class="knob" :default-val="0" knobText="Voice 2" backendId="shiftVoice2" />
      <MyKnobSplit class="knob" :default-val="0" knobText="Voice 3" backendId="shiftVoice3" />
    </div>
    <LPCknob class="LPCknob" backendId="enableLPC" />
    <img class="artwork" src="@/components/icons/artwork.png" />
  </div>
</template>

<script>
import MyKnob from '@/components/MyKnob.vue'
import MyKnobSplit from '@/components/MyKnobSplit.vue'
import LPCknob from '@/components/LPCknob.vue'

export default {
  components: {
    MyKnob,
    MyKnobSplit,
    LPCknob
  },
  methods: {}
}
</script>

<style scoped>
* {
  font-size: 13px;
  user-select: none;
  overflow: hidden;
}
.app {
  position: relative;
  background-color: #e1eaf3ff;
  width: 610px;
  height: 400px;
}
.knobs {
  position: absolute;
  bottom: 0;
  right: 55px;
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  grid-template-rows: repeat(2, auto);
  gap: 0 42px;
  z-index: 1;
}
.knob {
  margin-bottom: 65px;
  overflow: visible;
}
.LPCknob {
  position: absolute;
  left: 60px;
  top: 115px;
  z-index: 1;
}
.app {
  position: absolute;
  top: 0;
  left: 0;
  background-color: #e1eaf3ff;
}
.artwork {
  position: absolute;
  left: -220px;
  top: -60px;
  width: 180%;
  z-index: 0;
  transform: rotate(-40deg);
  opacity: 0.7;
}
</style>
