import React from 'react'
import type { DoseEvent } from '../../entities/pump/DoseEvent'

type Props = {
  doses: DoseEvent[]
}

const TodayDoses: React.FC<Props> = ({ doses }) => {
  return (
    <section>
      <h3>Today’s Doses</h3>
      {doses?.length ? (
        <ul>
          {doses.map((d) => {
            const ts = typeof d.timestamp === 'string' ? d.timestamp : new Date(d.timestamp).toLocaleString()
            return (
              <li key={d.id ?? d.pumpId}>{d.pumpId} - {ts} - {d.amountMl ?? 0} ml</li>
            )
          })}
        </ul>
      ) : (
        <div>No doses recorded today.</div>
      )}
    </section>
  )
}

export default TodayDoses
