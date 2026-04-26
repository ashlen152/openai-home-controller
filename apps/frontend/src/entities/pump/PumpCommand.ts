// Command issued to a specific pump
export interface PumpCommand {
  id: string
  pumpId: string
  command: string
  payload?: any
  createdAt?: string | Date
  status?: 'queued' | 'sent' | 'executed' | 'failed'
}
