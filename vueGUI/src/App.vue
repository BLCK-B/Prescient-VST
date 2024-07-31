<template>
  <div class="app">
    <div class="knobs">
      <MyKnob
        class="knobOrder"
        v-model="knobOrderVal"
        :default-val="50"
        knobText="Order"
        backendId="model order"
      />
      <MyKnob
        class="knobMono"
        v-model="knobMonoVal"
        :default-val="0"
        knobText="Mono"
        backendId="monostereo"
      />
      <MyKnob
        class="knobDryWet"
        v-model="knobDryWetVal"
        :default-val="0"
        knobText="Dry / wet"
        backendId="passthrough"
      />
      <MyKnobSplit class="knobVoice1" v-model="knobVoice1Val" :default-val="0" knobText="Voice 1" />
      <MyKnobSplit class="knobVoice2" v-model="knobVoice2Val" :default-val="0" knobText="Voice 2" />
      <MyKnobSplit class="knobVoice3" v-model="knobVoice3Val" :default-val="0" knobText="Voice 3" />
    </div>
  </div>
</template>

<script>
import MyKnob from '@/components/MyKnob.vue'
import MyKnobSplit from '@/components/MyKnobSplit.vue'

export default {
  components: {
    MyKnob,
    MyKnobSplit
  },
  data() {
    return {
      knobOrderVal: 0,
      knobMonoVal: 0,
      knobDryWetVal: 0,
      knobVoice1Val: 0,
      knobVoice2Val: 0,
      knobVoice3Val: 0
    }
  },
  methods: {}
}
</script>

<style scoped>
* {
  font-size: 15px;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
}
.knobs {
  width: 50%;
  display: grid;
  grid-template-columns: repeat(3, 1fr); /* 3 equal-width columns */
  grid-template-rows: repeat(2, auto); /* 2 rows with automatic height based on content */
  gap: 16px; /* Gap between items */
  padding: 16px; /* Optional padding around the grid */
}
.knobOrder,
.knobMono,
.knobDryWet,
.knobVoice1,
.knobVoice2,
.knobVoice3 {
  background-color: tranparent;
}
.app {
  position: absolute;
  top: 0;
  left: 0;
  background-color: #e1eaf3ff;
}
.knob {
  height: 200px;
  width: 200px;
  position: absolute;
  left: 200px;
  top: 150px;
}
</style>
