import React from 'react'

type Props = {
  logs: string[]
}

const SerialLogsViewer: React.FC<Props> = ({ logs }) => {
  return (
    <section>
      <h3>ESP32 Serial Logs</h3>
      <pre style={{ background: '#0b1020', color: '#e2e8f0', padding: 12, borderRadius: 6, maxHeight: 300, overflow: 'auto' }}>
        {logs.join('\n')}
      </pre>
    </section>
  )
}

export default SerialLogsViewer
