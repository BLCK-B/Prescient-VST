<template>
  <div class="knob-container">
    <div
        class="knob"
        @mousedown="startDragging"
        @wheel.prevent="handleScroll"
        @dblclick="resetToDefault"
    >
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
      <img class="knob-center-image" src="../components/icons/knobpic.png" alt="Knob Center" />
      <img
          class="knob-center-image"
          src="../components/icons/knobindicator.png"
          alt="Knob Center"
          :style="{ transform: 'translate(-50%, -50%) rotate(' + rotationAngle + 'deg)' }"
      />
    </div>
    <div class="knobText">
      <p>{{ knobText }}</p>
    </div>
  </div>
</template>

<script>
import * as Juce from '@/juce/index.js'

export default {
  props: {
    defaultVal: Number,
    knobText: String,
    backendId: String
  },
  mounted() {
    this.value = this.defaultVal
  },
  data() {
    return {
      value: 0,
      isDragging: false,
      startY: 0,
      startValue: 0,
      circumference: 2 * Math.PI * 45
    }
  },
  computed: {
    strokeOffset() {
      return this.circumference * (1 - this.value / 100)
    },
    rotationAngle() {
      return this.value * 3.6 + 180
    }
  },
  watch: {
    value(newVal) {
      this.sendToBackend(newVal)
    }
  },
  methods: {
    startDragging(event) {
      this.isDragging = true
      this.startY = event.clientY
      this.startValue = this.value
      document.addEventListener('mousemove', this.handleDrag)
      document.addEventListener('mouseup', this.stopDragging)
    },
    stopDragging() {
      this.isDragging = false
      document.removeEventListener('mousemove', this.handleDrag)
      document.removeEventListener('mouseup', this.stopDragging)
    },
    handleDrag(event) {
      if (this.isDragging) {
        const deltaMove = this.startY - event.clientY
        this.value = this.startValue + Math.round(deltaMove / 2)
        this.value = Math.max(0, Math.min(100, this.value))
      }
    },
    handleScroll(event) {
      if (event.target.closest('.knob')) {
        this.value += event.deltaY > 0 ? -2 : 2
        this.value = Math.max(0, Math.min(100, this.value))
        event.preventDefault()
      }
    },
    resetToDefault() {
      this.value = this.defaultVal
    },
    sendToBackend(newVal) {
      const sliderState = Juce.getSliderState(this.backendId)
      sliderState.setNormalisedValue(newVal / 100)
    }
  }
}
</script>

<style scoped>
* {
  transition: 0.03s;
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
  position: absolute;
  color: black;
  font-weight: bold;
  top: 105%;
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
