import React from 'react';
import type { Pump } from '../../entities/pump/Pump';

type Props = {
  pump: Pump;
  onEdit?: (pump: Pump) => void;
  onCalibrate?: (pumpId: string) => void;
  onDelete?: (pumpId: string) => void;
};

export const PumpCard: React.FC<Props> = ({ pump, onEdit, onCalibrate, onDelete }) => {
  const id = (pump as any).id ?? (pump as any).pumpId ?? '';
  const online = (pump as any).isOnline ?? true;
  const displayName = id ? `Pump ${id}` : 'Pump';
  const location = (pump as any).location ?? 'Unknown';
  const speed = (pump as any).speed ?? 0;

  return (
    <div className="pump-card" aria-label={`Pump ${displayName}`} style={{ border: '1px solid #e5e7eb', borderRadius: 8, padding: 12 }}>
      <div className="pump-card-header" style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
        <span className={`pump-status ${online ? 'online' : 'offline'}`} style={{ fontWeight: 600 }}>
          {online ? 'Online' : 'Offline'}
        </span>
        <strong className="pump-name">{displayName}</strong>
      </div>
      <div className="pump-card-body" style={{ marginTop: 8 }}>
        <div>Location: {location}</div>
        <div>Speed: {speed} ml/s</div>
      </div>
      <div className="pump-card-actions" style={{ display: 'flex', gap: 8, marginTop: 8 }}>
        <button type="button" onClick={() => onEdit?.(pump)}>Edit</button>
        <button type="button" onClick={() => onCalibrate?.(id)}>Calibrate</button>
        <button type="button" onClick={() => onDelete?.(id)}>Delete</button>
      </div>
    </div>
  );
};

export default PumpCard
