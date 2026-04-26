import React, { useEffect, useState } from 'react'
import { getPumps, getDoseEvents, getTodayDoseEvents } from '../../shared/api/pump'
import PumpDashboard from '../../widgets/pump-dashboard/PumpDashboard'
// Using a loose Pump type here to avoid cross-package TS mismatches between API types and UI types
type Pump = any
import type { DoseEvent } from '../../entities/pump/DoseEvent'

const PumpControllerPage: React.FC = () => {
  const [pumps, setPumps] = useState<Pump[]>([])
  const [loading, setLoading] = useState(true)
  const [todayDoses, setTodayDoses] = useState<DoseEvent[]>([])
  const [history, setHistory] = useState<DoseEvent[]>([])

  useEffect(() => {
    let mounted = true
    const fetchData = async () => {
      try {
        // Fetch pumps
        const pumpsRes = await getPumps()
        const pumpsList = (pumpsRes as any) as Pump[]
        if (mounted) {
          setPumps(pumpsList ?? [])
        }

        // Aggregate today's doses per pump
        const todayProms = pumpsList?.map(async (p) => {
          try {
            const t = await getTodayDoseEvents(p.pumpId ?? '')
            const total = (t as any).totalToday ?? 0
            const de: DoseEvent = {
              id: `${p.pumpId}-today`,
              pumpId: p.pumpId ?? '',
              timestamp: new Date().toISOString(),
              amountMl: total,
              event: 'complete',
            }
            return de
          } catch {
            return null
          }
        })
        const todays = await Promise.all(todayProms)
        setTodayDoses((todays.filter((x): x is DoseEvent => x !== null) as DoseEvent[]) ?? [])

        // Aggregate dose history for all pumps
        const histPromises = pumpsList?.map(async (p) => {
          try {
            const h = await getDoseEvents(p.pumpId ?? '')
            const items = ((h as any).doses ?? []) as any[]
            return items.map((it) => ({
              id: it.id ?? `${p.pumpId}-${it.timestamp}`,
              pumpId: p.pumpId ?? '',
              timestamp: it.timestamp ?? new Date().toISOString(),
              amountMl: it.amountMl ?? 0,
              event: it.event ?? 'complete',
            })) as DoseEvent[]
          } catch {
            return [] as DoseEvent[]
          }
        })
        const histArrays = await Promise.all(histPromises ?? [])
        const flat: DoseEvent[] = ([] as DoseEvent[]).concat(...histArrays)
        setHistory(flat)
      } catch {
        // swallow; show empty state
        setPumps([])
      } finally {
        if (mounted) setLoading(false)
      }
    }
    fetchData()
    return () => { mounted = false }
  }, [])

  // Simple navigation to pump dashboard at least renders
  return (
    <div style={{ padding: 16 }}>
      <h1>Pump Controller</h1>
      {loading ? (
        <div>Loading...</div>
      ) : (
        <PumpDashboard pumps={pumps} todayDoses={todayDoses} doseHistory={history} />
      )}
    </div>
  )
}

export default PumpControllerPage
export { PumpControllerPage }
