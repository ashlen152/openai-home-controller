// Pump settings contract (subset aligned with server API expectations)
export interface PumpSettings {
  dailyVolume: number
  dayStartHour: number
  dayEndHour: number
  dayPercent: number
  stepsPerML: number
  activeProfile: number
}
