<template>
  <div class="app">
    <div class="knobs">
      <MyKnob class="knob" :default-val="50" knobText="Order" backendId="model order" />
      <MyKnob class="knob" :default-val="0" knobText="Mono" backendId="monostereo" />
      <MyKnob class="knob" :default-val="0" knobText="Dry / wet" backendId="passthrough" />
      <MyKnobSplit class="knob" :default-val="0" knobText="Voice 1" backendId="shiftVoice1" />
      <MyKnobSplit class="knob" :default-val="0" knobText="Voice 2" backendId="shiftVoice2" />
      <MyKnobSplit class="knob" :default-val="0" knobText="Voice 3" backendId="shiftVoice3" />
    </div>
    <LPCknob class="LPCknob" backendId="enableLPC" />
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
  font-size: 15px;
}
.app {
  position: relative;
  background-color: #e1eaf3ff;
  width: 800px;
  height: 500px;
}
.knobs {
  position: absolute;
  bottom: 0px;
  right: 60px;
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  grid-template-rows: repeat(2, auto);
  gap: 0px 40px;
}
.knob {
  margin-bottom: 65px;
}
.LPCknob {
  position: absolute;
  left: 160px;
  top: 200px;
}
.app {
  position: absolute;
  top: 0;
  left: 0;
  background-color: #e1eaf3ff;
}
</style>
