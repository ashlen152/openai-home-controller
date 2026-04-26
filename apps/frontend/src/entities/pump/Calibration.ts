// Calibration record for a pump
export interface Calibration {
  id: string
  pumpId?: string
  date: string | Date
  value: number
  notes?: string
}
