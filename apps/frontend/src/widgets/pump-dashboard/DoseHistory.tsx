import React from 'react'
import type { DoseEvent } from '../../entities/pump/DoseEvent'

type Props = {
  history: DoseEvent[]
}

const DoseHistory: React.FC<Props> = ({ history }) => {
  if (!history?.length) return <div>No dose history available.</div>
  return (
    <section>
      <h3>Dose History</h3>
      <ul>
        {history.map((h) => {
          const ts = typeof (h as any).timestamp === 'string' ? (h as any).timestamp : new Date((h as any).timestamp).toLocaleString()
          return (
            <li key={`${(h as any).pumpId ?? ''}-${(h as any).timestamp ?? ''}`}>
              {(h as any).pumpId} - {ts} - {(h as any).amountMl ?? 0} ml
            </li>
          )
        })}
      </ul>
    </section>
  )
}

export default DoseHistory
