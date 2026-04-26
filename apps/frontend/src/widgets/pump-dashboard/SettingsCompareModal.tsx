import React from 'react'

type Props = {
  onClose?: () => void
  onCompare?: () => void
}

const SettingsCompareModal: React.FC<Props> = ({ onClose, onCompare }) => {
  return (
    <div style={{ padding: 16, border: '1px solid #e5e7eb', borderRadius: 6 }}>
      <div style={{ fontWeight: 600, marginBottom: 8 }}>Settings Comparison</div>
      <button onClick={onCompare}>Compare</button>
      <button onClick={onClose} style={{ marginLeft: 8 }}>Close</button>
    </div>
  )
}

export default SettingsCompareModal
