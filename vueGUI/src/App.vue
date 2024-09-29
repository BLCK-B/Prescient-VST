<template>
  <div class="app">
    <div class="knobs">
      <MyKnob class="knob" :default-val="100" knobText="Order" backendId="model order" />
      <MyKnob class="knob" :default-val="50" knobText="Mono / stereo" backendId="monostereo" />
      <MyKnob class="knob" :default-val="100" knobText="Dry / wet" backendId="passthrough" />
      <MyKnobSplit class="knob" :default-val="0" knobText="Voice 1" backendId="shiftVoice1" />
      <MyKnobSplit class="knob" :default-val="0" knobText="Voice 2" backendId="shiftVoice2" />
      <MyKnobSplit class="knob" :default-val="0" knobText="Voice 3" backendId="shiftVoice3" />
    </div>
    <LPCknob class="LPCknob" backendId="enableLPC" />
    <img class="demotext" src="@/components/icons/demotext.png" />
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
}
.app {
  position: relative;
  background-color: #e1eaf3ff;
  width: 630px;
  height: 420px;
}
.knobs {
  position: absolute;
  bottom: 0;
  right: 55px;
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  grid-template-rows: repeat(2, auto);
  gap: 0 42px;
}
.knob {
  margin-bottom: 65px;
}
.LPCknob {
  position: absolute;
  left: 85px;
  top: 133px;
}
.app {
  position: absolute;
  top: 0;
  left: 0;
  background-color: #e1eaf3ff;
}
.demotext {
  position: absolute;
  left: 40px;
  top: 30px;
  width: 140px;
  opacity: 0.25;
}
</style>
