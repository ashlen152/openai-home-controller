// Dose event emitted by the pump during delivery
export interface DoseEvent {
  id: string
  pumpId: string
  timestamp: string | Date
  amountMl?: number
  event?: 'start' | 'complete' | 'pause' | 'resume'
}
