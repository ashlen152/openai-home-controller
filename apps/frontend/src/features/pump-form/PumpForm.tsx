import React, { useEffect, useState } from 'react'
import type { Pump } from '../../entities/pump'

type Props = {
  value?: Partial<Pump>
  onSubmit?: (payload: Partial<Pump>) => void
}

const PumpForm: React.FC<Props> = ({ value = {}, onSubmit }) => {
  const [pumpId, setPumpId] = useState<string>(value.pumpId ?? '')
  const [dailyVolume, setDailyVolume] = useState<number>(value.dailyVolume ?? 0)
  const [startHour, setStartHour] = useState<number>(value.dayStartHour ?? 0)
  const [endHour, setEndHour] = useState<number>(value.dayEndHour ?? 24)
  const [stepsPerML, setStepsPerML] = useState<number>(value.stepsPerML ?? 1)
  const [activeProfile, setActiveProfile] = useState<number>(value.activeProfile ?? 0)

  useEffect(() => {
    if (value?.pumpId) {
      setPumpId(value.pumpId ?? '')
      setDailyVolume((value as any).dailyVolume ?? 0)
      setStartHour((value as any).dayStartHour ?? 0)
      setEndHour((value as any).dayEndHour ?? 24)
      setStepsPerML((value as any).stepsPerML ?? 1)
      setActiveProfile((value as any).activeProfile ?? 0)
    }
  }, [value])

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault()
    if (onSubmit) {
      onSubmit({
        pumpId,
        dailyVolume,
        dayStartHour: startHour,
        dayEndHour: endHour,
        stepsPerML,
        activeProfile,
      })
    }
  }

  return (
    <form onSubmit={handleSubmit}>
      <div>
        <label>Pump ID</label>
        <input value={pumpId} onChange={(e) => setPumpId(e.target.value)} />
      </div>
      <div>
        <label>Daily Volume (ml)</label>
        <input type="number" value={dailyVolume} onChange={(e) => setDailyVolume(Number(e.target.value))} />
      </div>
      <div>
        <label>Day Start Hour</label>
        <input type="number" value={startHour} onChange={(e) => setStartHour(Number(e.target.value))} />
      </div>
      <div>
        <label>Day End Hour</label>
        <input type="number" value={endHour} onChange={(e) => setEndHour(Number(e.target.value))} />
      </div>
      <div>
        <label>Steps per mL</label>
        <input type="number" value={stepsPerML} onChange={(e) => setStepsPerML(Number(e.target.value))} />
      </div>
      <div>
        <label>Active Profile</label>
        <input type="number" value={activeProfile} onChange={(e) => setActiveProfile(Number(e.target.value))} />
      </div>
      <button type="submit">Save Pump</button>
    </form>
  )
}

export default PumpForm
