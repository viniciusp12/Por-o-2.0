Vou criar um arquivo `README.md` no estilo do `README` do "Antivírus Porão", adaptado para o código em C que desenvolvemos. O `README` será claro, com seções sobre o propósito do projeto, como compilar e executar o código, e como testá-lo. Vou manter o tom e a estrutura semelhantes ao do "Porão", com um toque de humor e emojis, como no original.

---

# 📛 PoraoRansomwareDetector (C Version) 📛

Bem-vindo ao **PoraoRansomwareDetector**, a versão em C do nosso querido "Antivírus Porão"! 🚪🔒  
Se você já conhece o Porão em Python, essa é uma reimaginação mais leve e direta, feita para detectar e combater ransomwares em desktops Windows. Ele monitora processos suspeitos, alterações no registro, mudanças em pastas críticas e até faz backup dos seus arquivos mais importantes! 🛡️

> **Aviso**: Este é um projeto educacional. Use com cuidado e **NUNCA** execute ransomwares reais fora de um ambiente isolado (como uma máquina virtual). Não nos responsabilizamos por qualquer dano. 😅

---

## 🎯 O que ele faz?
O **PoraoRansomwareDetector** é um programa em C que roda em segundo plano e protege seu desktop Windows contra ransomwares. Ele:

- **Monitora Processos Suspeitos** 🕵️‍♂️: Fica de olho em processos como `vssadmin.exe`, `wmic.exe`, `bcdedit.exe`, e `taskkill.exe`, que são comumente usados por ransomwares para bagunçar o sistema. Se detectar um, ele encerra na hora!
- **Vigia o Registro** 🔍: Observa mudanças em chaves como `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run` e `RunOnce`, onde ransomwares podem tentar se tornar "imortais".
- **Protege Pastas Críticas** 📂: Monitora as pastas `Downloads`, `Documents`, e `Desktop`, criando arquivos honeypot (`.poraoX.txt`) para enganar o ransomware. Se ele mexer nesses arquivos, o Porão percebe!
- **Faz Backup e Restaura** 💾: Cria backups automáticos das pastas críticas a cada 1h30 e restaura os arquivos se um ransomware for detectado.
- **Registra Tudo** 📜: Gera logs detalhados em `antivirus_log.txt` para você acompanhar tudo o que está acontecendo.

---

## 🛠️ Como Compilar e Executar

### Pré-requisitos
- Um sistema Windows (testado no Windows 10, mas deve funcionar em outras versões também).
- Um compilador C para Windows, como:
  - **Visual Studio** (recomendado, com o compilador MSVC).
  - **MinGW** (uma alternativa leve).
- Privilégios de administrador (necessário para acessar algumas pastas e encerrar processos).

### Passo a Passo
1. **Clone o Repositório** 📥  
   Baixe o código do GitHub:
   ```bash
   git clone https://github.com/seu-usuario/PoraoRansomwareDetector.git
   cd PoraoRansomwareDetector
   ```

2. **Compile o Código** 🔨  
   - **Com Visual Studio (Prompt de Desenvolvedor)**: Abra o "Prompt de Desenvolvedor para Visual Studio" e execute:
     ```cmd
     cl ransomware_detector.c
     ```
     Isso gerará o executável `ransomware_detector.exe`.
   - **Com MinGW**: Se estiver usando MinGW, compile com:
     ```cmd
     gcc ransomware_detector.c -o ransomware_detector.exe
     ```

3. **Execute o Programa** 🚀  
   - Certifique-se de executar como administrador para evitar problemas de permissão:
     ```cmd
     ransomware_detector.exe
     ```
   - Você verá a mensagem: "Programa em execução. Pressione Ctrl+C para parar."
   - O programa começará a monitorar suas pastas, criar backups e registrar tudo em `antivirus_log.txt`.

---

## 📂 Estrutura do Projeto
- `ransomware_detector.c`: O código principal em C, com todas as funcionalidades de monitoramento, backup e restauração.
- `antivirus_log.txt`: Arquivo de log gerado automaticamente, onde todas as ações (detecções, backups, restaurações) são registradas.

---

## 🧪 Como Testar
**Atenção**: Testar com ransomwares reais é EXTREMAMENTE arriscado. Faça isso apenas em uma máquina virtual isolada (ex.: VirtualBox ou VMware), com a rede desativada e um snapshot para restaurar o sistema. 🛑

### Teste Simples (Sem Ransomware Real)
1. **Verifique os Honeypots** 🪤  
   - O programa cria arquivos `.poraoX.txt` (ex.: `.porao1.txt`, `.porao2.txt`) nas pastas `Downloads`, `Documents`, e `Desktop`. Abra essas pastas e confirme que os arquivos estão lá.
   
2. **Teste o Monitoramento de Processos** 💻  
   - Abra um prompt de comando e execute:
     ```cmd
     vssadmin list shadows
     ```
   - O programa deve detectar o `vssadmin.exe`, encerrá-lo e registrar no log: "Processo suspeito detectado: vssadmin.exe (PID: X)".

3. **Teste o Monitoramento de Arquivos** 📄  
   - Modifique ou exclua um arquivo honeypot (ex.: `C:\Users\SeuUsuario\Downloads\.porao1.txt`). O programa deve registrar: "Mudança detectada em C:\Users\SeuUsuario\Downloads, possível ransomware."

4. **Teste o Monitoramento do Registro** 🔧  
   - Adicione uma entrada ao registro:
     ```cmd
     reg add "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run" /v Teste /t REG_SZ /d "notepad.exe"
     ```
   - O programa deve registrar: "Chave de registro alterada: HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run, possível ransomware."

5. **Verifique os Backups** 💾  
   - O programa cria backups das pastas críticas a cada 1h30 em `C:\Users\SeuUsuario\ProtectedBackup`. Confirme que as pastas `Downloads`, `Documents`, e `Desktop` estão lá.

### Teste com Ransomware (Ambiente Isolado)
1. **Configure uma Máquina Virtual** 🖥️  
   - Use uma VM com Windows (ex.: Windows 10).
   - Desative a rede para evitar que o ransomware se espalhe.
   - Crie um snapshot para restaurar o sistema após o teste.

2. **Obtenha uma Amostra de Ransomware** 🦠  
   - Use uma amostra conhecida, como o WannaCry, de um repositório seguro como [MalwareBazaar](https://bazaar.abuse.ch/) ou [theZoo](https://github.com/ytisf/theZoo).
   - **NUNCA** execute isso em seu computador real!

3. **Execute o Programa** 🚀  
   - Compile e execute o `ransomware_detector.exe` na VM como administrador.

4. **Execute o Ransomware** ⚠️  
   - Coloque o ransomware em uma pasta monitorada (ex.: `Downloads`) e execute-o.
   - O programa deve:
     - Detectar processos suspeitos (ex.: `vssadmin.exe`) e encerrá-los.
     - Registrar mudanças nos arquivos honeypot.
     - Detectar alterações no registro.
     - Restaurar os backups automaticamente.

5. **Verifique os Logs** 📜  
   - Abra o `antivirus_log.txt` para confirmar as detecções e ações:
     - "Processo suspeito detectado: vssadmin.exe (PID: X)"
     - "Mudança detectada em C:\Users\SeuUsuario\Downloads, possível ransomware."
     - "Ransomware detectado! Restaurando backups..."
     - "Backup restaurado: C:\Users\SeuUsuario\ProtectedBackup\Downloads -> C:\Users\SeuUsuario\Downloads"

---

## 🐛 Limitações e Possíveis Melhorias
- **Restauração Manual**: Embora o código restaure backups automaticamente quando detecta um ransomware, o intervalo de backup (1h30) pode ser longo. Se o ransomware agir rápido, você pode perder algumas alterações recentes.
- **Processos Suspeitos**: O programa monitora apenas alguns processos específicos (`vssadmin.exe`, `wmic.exe`, etc.). Ransomwares mais avançados podem usar outros métodos.
- **Proteção do Backup**: Os backups não estão protegidos contra exclusão pelo ransomware. Você pode adicionar permissões com `icacls` para negar acesso.
- **Alerta Gráfico**: Não há um alerta visual (como no Porão em Python). Adicionar um `MessageBox` seria legal para avisar o usuário.
- **Mais Detecções**: O programa poderia monitorar mais chaves de registro ou detectar padrões de comportamento (ex.: alta taxa de modificação de arquivos).

---

## 🤝 Contribuições
Quer ajudar a melhorar o PoraoRansomwareDetector? 💡  
- Faça um fork do repositório.
- Adicione suas melhorias (ex.: mais detecções, proteção de backups, interface gráfica).
- Envie um pull request! 🚀

---

## 📜 Licença
Este projeto é para fins educacionais e não tem uma licença oficial. Use por sua conta e risco! 😅

---

## 🎉 Agradecimentos
- Inspirado no nosso querido **Antivírus Porão** em Python. 🐍
- Feito com 💙 por [Seu Nome ou Equipe].

**Proteja seu PC com o Porão (agora em C)!** 🚪🔒
