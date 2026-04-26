// Pump entity representing a single pump configuration/status
import type { Calibration } from './Calibration'

export interface Pump {
  pumpId: string
  enabled: boolean
  dailyVolume: number
  dayStartHour: number
  dayEndHour: number
  dayPercent: number
  stepsPerML: number
  activeProfile: number
  pausedUntil?: number
  lastSync?: string | Date
  calibrationHistory?: Calibration[]
  createdAt?: string | Date
  updatedAt?: string | Date
  online: boolean
  lastHeartbeat?: number
  wifiRssi?: number
  ipAddress?: string
  isDosing: boolean
  totalDosedToday: number
  uptimeSeconds?: number
  freeHeap?: number
  lastSettingsSync?: number
  settingsMatch: boolean
  mismatches: string[]
}

export type PumpPosix = Pump;
