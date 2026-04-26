import React from 'react'
import type { Pump } from '../../entities/pump'
import PumpList from './PumpList'
import TodayDoses from './TodayDoses'
import DoseHistory from './DoseHistory'
import type { DoseEvent } from '../../entities/pump/DoseEvent'

type Props = {
  pumps: Pump[]
  todayDoses?: DoseEvent[]
  doseHistory?: DoseEvent[]
}

const PumpDashboard: React.FC<Props> = ({ pumps, todayDoses = [], doseHistory = [] }) => {
  // Pretty simple aggregation that renders the main list and a couple of readouts
  return (
    <section>
      <h2>Pump Dashboard</h2>
      <PumpList pumps={pumps} />
      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 24, marginTop: 24 }}>
        <TodayDoses doses={todayDoses} />
        <DoseHistory history={doseHistory} />
      </div>
    </section>
  )
}

export default PumpDashboard
