import React from 'react'
import type { Pump } from '../../entities/pump'
import PumpCard from './PumpCard'

type Props = {
  pumps: Pump[]
}

const PumpList: React.FC<Props> = ({ pumps }) => {
  if (!pumps?.length) return <div>No pumps configured.</div>
  return (
    <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(240px, 1fr))', gap: 16 }}>
      {pumps.map((p) => (
        <PumpCard key={p.pumpId} pump={p} />
      ))}
    </div>
  )
}

export default PumpList
