<template>
  <div class="knob-container">
    <div class="knob" @click="toggle">
      <svg class="knob-svg" viewBox="0 0 100 100">
        <circle class="knob-bg" :class="{ knobInactive: value === 0 }" cx="50" cy="50" r="45" />
        <circle
          class="knob-indicator"
          cx="50"
          cy="50"
          r="45"
          :stroke-dasharray="circumference"
          :stroke-dashoffset="strokeOffset"
        />
      </svg>
      <img
        v-if="this.value == 100"
        class="knob-center-image"
        src="../components/icons/lpcon.png"
        alt="Knob Center"
      />
      <img
        v-else
        class="knob-center-image"
        src="../components/icons/lpcoff.png"
        alt="Knob Center"
      />
    </div>
  </div>
</template>

<script>
import * as Juce from '@/juce/index.js'

export default {
  props: {
    backendId: String
  },
  mounted() {
    const saved = localStorage.getItem(this.backendId)
    this.value = saved !== null ? parseFloat(saved) : 0
  },
  data() {
    return {
      value: 0,
      circumference: 2 * Math.PI * 45
    }
  },
  computed: {
    strokeOffset() {
      return this.circumference * (1 - this.value / 100)
    }
  },
  methods: {
    toggle() {
      this.value = this.value == 100 ? 0 : 100
      this.sendToBackend()
    },
    sendToBackend() {
      localStorage.setItem(this.backendId, this.value);
      const sliderState = Juce.getSliderState(this.backendId)
      sliderState.setNormalisedValue(this.value / 100)
    }
  }
}
</script>

<style scoped>
* {
  user-select: none;
}
.knob-container {
  display: flex;
  justify-content: center;
  align-items: center;
  height: 200px;
}

.knob {
  position: relative;
  width: 105px;
  height: 105px;
  cursor: pointer;
}

.knob-svg {
  width: 100%;
  height: 100%;
  transform: rotate(90deg);
}

.knob-bg {
  transition: 0.4s;
  fill: none;
  stroke: #b8bcc1ff;
  stroke-width: 5;
}

.knobText {
  position: relative;
  color: black;
  font-weight: bold;
  top: 80px;
}

.knobInactive {
  stroke: #c7cbceff;
}

.knob-indicator {
  fill: none;
  stroke: #fd6c00;
  stroke-width: 5.5;
  stroke-linecap: round;
  stroke-dasharray: 282.74;
}

.knob-value {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  font-size: 24px;
  font-weight: bold;
  color: black;
}

.knob-center-image {
  position: absolute;
  top: 50%;
  left: 50%;
  width: 135%;
  height: 135%;
  transform: translate(-50%, -50%);
  pointer-events: none;
}
</style>
