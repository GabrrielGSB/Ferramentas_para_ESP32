#!/usr/bin/env python3
"""
Leitor serial para MemoryProfiler do ESP32.
Salva cada JSON recebido (delimitado por [MP_JSON_START]...[MP_JSON_END])
em um arquivo .jsonl com timestamp no nome.
"""

import serial
import sys
import os
import json
import argparse
from datetime import datetime

def main():
    parser = argparse.ArgumentParser(description='Captura JSONs do MemoryProfiler via serial')
    parser.add_argument('port',                                       help='Porta serial (ex: COM3, /dev/ttyUSB0)')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='Baud rate (padrão: 115200)')
    parser.add_argument('-o', '--output-dir', default='./data',       help='Diretório de saída (padrão: ./data)')
    parser.add_argument('--timeout', type=float, default=1,           help='Timeout da serial (s)')
    args = parser.parse_args()

    # Cria diretório se não existir
    os.makedirs(args.output_dir, exist_ok=True)

    # Gera nome do arquivo com timestamp
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    filename = f'memory_{timestamp}.jsonl'
    filepath = os.path.join(args.output_dir, filename)

    print(f'Iniciando captura. Arquivo: {filepath}')
    print(f'Aguardando dados na porta {args.port} ({args.baudrate} baud)...')
    print('Pressione Ctrl+C para parar.\n')

    try:
        with serial.Serial(args.port, args.baudrate, timeout=args.timeout) as ser:
            with open(filepath, 'a', encoding='utf-8') as f:
                buffer_line = ""
                while True:
                    try:
                        # Lê linha da serial (bloqueia até timeout se não houver dados)
                        line = ser.readline()
                        if not line:
                            continue

                        # Decodifica tratando possíveis erros de encoding
                        try:
                            decoded = line.decode('utf-8').strip()
                        except UnicodeDecodeError:
                            decoded = line.decode('latin-1').strip()

                        if not decoded:
                            continue

                        # Verifica se a linha contém nossos marcadores
                        start_marker = "[MP_JSON_START]"
                        end_marker = "[MP_JSON_END]"
                        if start_marker in decoded and end_marker in decoded:
                            # Extrai apenas o JSON entre os marcadores
                            start_idx = decoded.index(start_marker) + len(start_marker)
                            end_idx = decoded.index(end_marker)
                            json_str = decoded[start_idx:end_idx].strip()

                            # Valida se é um JSON válido (e embeleza para uma linha)
                            try:
                                data = json.loads(json_str)
                                # Escreve no arquivo como uma linha JSON (formato JSON Lines)
                                f.write(json.dumps(data, ensure_ascii=False) + '\n')
                                f.flush()  # garante que seja escrito imediatamente
                                print(f'✔ {json_str[:80]}...')  # mostra snippet
                            except json.JSONDecodeError as e:
                                print(f'✖ JSON inválido ignorado: {e}')

                    except KeyboardInterrupt:
                        print('\n\nCaptura interrompida.')
                        break
                    except Exception as e:
                        print(f'Erro: {e}')
                        continue

    except serial.SerialException as e:
        print(f'Erro ao abrir porta serial: {e}')
        sys.exit(1)

    print(f'Arquivo salvo: {filepath}')

if __name__ == '__main__':
    main()