import React from 'react'

type Props = {
  pumpId?: string
  onClose?: () => void
  onCalibrate?: (pumpId: string) => void
}

const CalibrationModal: React.FC<Props> = ({ pumpId, onClose, onCalibrate }) => {
  return (
    <div style={{ padding: 16, border: '1px solid #e5e7eb', borderRadius: 6 }}>
      <div style={{ fontWeight: 600, marginBottom: 8 }}>Calibration</div>
      <button onClick={() => onCalibrate?.(pumpId ?? '')}>Start Calibration</button>
      <button onClick={onClose} style={{ marginLeft: 8 }}>Close</button>
    </div>
  )
}

export default CalibrationModal
