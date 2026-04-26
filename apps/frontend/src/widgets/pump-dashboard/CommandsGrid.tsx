import React from 'react'
import type { PumpCommand } from '../../entities/pump'

type Props = {
  commands: PumpCommand[]
}

const CommandsGrid: React.FC<Props> = ({ commands }) => {
  return (
    <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(180px, 1fr))', gap: 12 }}>
      {commands.map((c) => (
        <div key={c.id} style={{ padding: 8, border: '1px solid #e5e7eb', borderRadius: 6 }}>{c.command}</div>
      ))}
    </div>
  )
}

export default CommandsGrid
