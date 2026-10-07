# ---------------------------------------------------------------------------
#  QuizMaster - automated regression harness (Windows PowerShell 5.1).
#
#  Every case launches build\QuizMaster.exe against a throw-away copy of the
#  data folder in build\test-runs\, types a scripted sequence of keys and then
#  checks the printed output plus the resulting data files.
#
#      powershell -ExecutionPolicy Bypass -File docs\run-tests.ps1
#
#  Build the project first:  build.bat   (or)  make
#  The full written-up result is in docs\TEST_REPORT.md.
# ---------------------------------------------------------------------------
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$exe  = Join-Path $root 'build\QuizMaster.exe'
$tmp  = Join-Path $root 'build\test-runs'

function Records([string]$file) {
    if (-not (Test-Path $file)) { return 0 }
    return @(Get-Content -LiteralPath $file | Where-Object { $_ -notmatch '^\s*#' -and $_.Trim() -ne '' }).Count
}

function Run-Case {
    param([string]$Name, [string[]]$Keys, [switch]$Fresh, [string[]]$Expect = @(),
          [string]$ExpectNot = '', [string]$DataFile = '', [string]$DataPattern = '')

    $dir = Join-Path $tmp $Name
    if (Test-Path $dir) { Remove-Item $dir -Recurse -Force }
    New-Item -ItemType Directory -Path $dir | Out-Null
    if (-not $Fresh) { Copy-Item -Path (Join-Path $root 'data\*') -Destination $dir }

    if ($DataFile -ne '') {
        Add-Content -LiteralPath (Join-Path $dir $DataFile) -Value "`r`nGARBAGE-NOT-A-RECORD`r`nQ|xx|bad|row|with|too|few|Q`r`n"
    }

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName  = $exe
    $psi.Arguments = "--data-dir `"$dir`" --no-color"
    $psi.RedirectStandardInput  = $true
    $psi.RedirectStandardOutput = $true
    $psi.UseShellExecute = $false
    $p = [System.Diagnostics.Process]::Start($psi)
    $p.StandardInput.Write((($Keys -join "`r`n") + "`r`n"))
    $p.StandardInput.Close()
    $out = $p.StandardOutput.ReadToEnd()
    $p.WaitForExit()
    ($out -replace "`e\[[0-9;]*[A-Za-z]", '') | Set-Content -LiteralPath (Join-Path $dir '_output.txt') -Encoding ASCII

    $fails = @()
    foreach ($e in $Expect) { if ($out -notmatch [regex]::Escape($e)) { $fails += "missing: $e" } }
    if ($ExpectNot -ne '' -and $out -match [regex]::Escape($ExpectNot)) { $fails += "unexpected: $ExpectNot" }
    if ($p.ExitCode -ne 0) { $fails += "exit code $($p.ExitCode)" }

    [pscustomobject]@{
        Name = $Name; Pass = ($fails.Count -eq 0); Fails = $fails; Exit = $p.ExitCode
        Users = Records (Join-Path $dir 'users.txt'); Questions = Records (Join-Path $dir 'questions.txt')
        Quizzes = Records (Join-Path $dir 'quizzes.txt'); Results = Records (Join-Path $dir 'results.txt')
        Files = (@(Get-ChildItem -LiteralPath $dir -Filter *.txt | Where-Object { $_.Name -notlike '_*' }).Name) -join ','
        DataHit = if ($DataPattern -ne '') { @(Get-Content -LiteralPath (Join-Path $dir $DataFile) -ErrorAction SilentlyContinue |
                    Where-Object { $_ -match $DataPattern }).Count } else { 0 }
        Output = $out; Dir = $dir
    }
}

if (Test-Path $tmp) { Remove-Item $tmp -Recurse -Force }
New-Item -ItemType Directory -Path $tmp | Out-Null

$T = @()

$T += Run-Case -Name 't01-first-run' -Fresh -Keys @('4','','5') `
    -Expect @('System ready','SYSTEM STATUS','users.txt',"initial account 'admin' was created",'Goodbye')

$T += Run-Case -Name 't02-login-retry' -Keys @(
    '1','2','rahul','wrongpassword','ghost','whatever','2','rahul','rahul123','5','5') `
    -Expect @('Invalid username or password.','Login successful. Welcome, Rohan Verma!',
              'STUDENT DASHBOARD','Returned from the STUDENT portal')

$T += Run-Case -Name 't03-attempt-quiz' -Keys @(
    '1','2','rahul','rahul123','2','1','y',
    'Z','q','5','B','D','C','A','C',
    '5','5') `
    -Expect @('Answer must be A, B, C or D','Correct!  Answer: B','Wrong.    Correct answer: C',
              'Your result has been saved (Result ID 6)','QUIZ COMPLETED',
              'Percentage       : 40.00%','PASS (pass mark 40%)')

$T += Run-Case -Name 't04-register' -Keys @(
    '1','1','   ','Amit Sharma','amit sharma','rahul','amit','12','amit123','amit124','amit123','amit123','n','',
    '1','Kavya Nair','kavya','kavya123','kavya123','y','','3','5') `
    -Expect @('This field cannot be empty.',
              'The username must be 3-20 characters long',
              'Username already exists.',
              'The password must contain at least 4 characters.',
              'The two passwords do not match.',
              'Registration cancelled. Nothing was saved.',
              'Account created successfully','STU003')

$T += Run-Case -Name 't05-admin-add-question' -Keys @(
    '2','1','admin','admin123',
    '2','Which keyword declares a read-only variable?','const','constexpr','final','mutable','B','y','n',
    '3','','8','5') `
    -Expect @('Question Q15 added to the question bank','Which keyword declares a read-only variable?')

$T += Run-Case -Name 't06-admin-update-question' -Keys @(
    '2','1','admin','admin123',
    '4','1','UPDATED: which OOP principle hides internal data?','','','','','','y','',
    '3','','8','5') `
    -Expect @('updated','UPDATED: which OOP principle hides internal data?','quiz')

$T += Run-Case -Name 't07-admin-delete-question' -Keys @(
    '2','1','admin','admin123',
    '5','1','y','',
    '3','','8','5') `
    -Expect @('PLEASE CONFIRM','deleted','removed from 1 quiz(es)')

$T += Run-Case -Name 't08-admin-create-quiz' -Keys @(
    '2','1','admin','admin123',
    '1','C++ Basics Demo','Five basic questions','y',
    '14','3',
    '','','','9','','8','5') `
    -Expect @('Quiz "C++ Basics Demo" created with ID 3.','Question Q14 added to "C++ Basics Demo"',
              'draft (no questions)')

$T += Run-Case -Name 't09-admin-reports' -Keys @(
    '2','1','admin','admin123','6','n','','7','','8','5') `
    -Expect @('STUDENT RESULTS','QUIZ STATISTICS')

$T += Run-Case -Name 't10-student-results' -Keys @(
    '1','2','priya','priya123','3','y','1','','5','5') `
    -Expect @('MY RESULTS','RESULT DETAILS','QUIZ COMPLETED','Excellent','CORRECT')

$T += Run-Case -Name 't11-role-guard' -Keys @(
    '1','2','rahul','rahul123','6','7','8','0','-1','abc','5','5') `
    -Expect @('Please enter a number between 1 and 5.','Invalid input. Please enter a valid number.')

$T += Run-Case -Name 't12-invalid-input' -Keys @('abc','9','0','-3','2.5','x1','3','','5') `
    -Expect @('ABOUT THE PROJECT','Invalid input. Please enter a valid number.',
              'Please enter a number between 1 and 5.')

$T += Run-Case -Name 't13-corrupt-records' -DataFile 'questions.txt' -DataPattern 'UPDATED-MARKER' -Keys @(
    '2','1','admin','admin123','3','','8','5')

$T += Run-Case -Name 't14-status-and-profile' -Keys @(
    '4','','2','1','admin','admin123','3','','8','5') `
    -Expect @('SYSTEM STATUS','ADMIN DASHBOARD','Content management')

# ---- T15 unknown CLI option
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $exe; $psi.Arguments = '--bogus'; $psi.RedirectStandardOutput = $true
$psi.UseShellExecute = $false
$p = [System.Diagnostics.Process]::Start($psi); $o = $p.StandardOutput.ReadToEnd(); $p.WaitForExit()
$T += [pscustomobject]@{ Name='t15-bad-cli-option'; Pass = ($p.ExitCode -eq 1 -and $o -match 'Unknown option'); Fails=@()
                        Exit=$p.ExitCode; Users=0; Questions=0; Quizzes=0; Results=0; Files='-'; DataHit=0
                        Output=$o; Dir='' }

# ---- T16 corrupt questions.txt rows are skipped, valid ones still load
$dir = Join-Path $tmp 't16-corrupt-skip'
New-Item -ItemType Directory -Path $dir | Out-Null
Copy-Item -Path (Join-Path $root 'data\*') -Destination $dir
Add-Content -LiteralPath (Join-Path $dir 'questions.txt') -Value "`r`nGARBAGE`r`nQ|bad|only|two`r`n"
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName=$exe; $psi.Arguments="--data-dir `"$dir`" --no-color"
$psi.RedirectStandardInput=$true; $psi.RedirectStandardOutput=$true; $psi.UseShellExecute=$false
$p=[System.Diagnostics.Process]::Start($psi)
$p.StandardInput.Write("2`r`n1`r`nadmin`r`nadmin123`r`n3`r`n`r`n8`r`n5`r`n"); $p.StandardInput.Close()
$o=$p.StandardOutput.ReadToEnd(); $p.WaitForExit()
$okCorrupt = ($o -match 'Q14') -and ($o -match 'Total questions:\s+14')
$T += [pscustomobject]@{ Name='t16-corrupt-skip'; Pass=$okCorrupt; Fails=@(); Exit=$p.ExitCode; Users=3
                        Questions=0; Quizzes=0; Results=0; Files='-'; DataHit=0; Output=$o; Dir=$dir }

# ---- T17 delimiters inside a field survive a save/load round trip
$dir = Join-Path $tmp 't17-escaped-fields'
New-Item -ItemType Directory -Path $dir | Out-Null
Copy-Item -Path (Join-Path $root 'data\*') -Destination $dir
$tricky = 'What if a|b and c\d matter?'
$keys1 = @('2','1','admin','admin123','2',$tricky,'pipe | inside','back \ inside','plain','other','A','y','n','','3','','8','5') -join "`r`n"
$keys2 = @('2','1','admin','admin123','3','','8','5') -join "`r`n"
$o = ''
foreach ($k in @($keys1, $keys2)) {
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName=$exe; $psi.Arguments="--data-dir `"$dir`" --no-color"
    $psi.RedirectStandardInput=$true; $psi.RedirectStandardOutput=$true; $psi.UseShellExecute=$false
    $p=[System.Diagnostics.Process]::Start($psi)
    $p.StandardInput.Write($k + "`r`n"); $p.StandardInput.Close()
    $o += $p.StandardOutput.ReadToEnd(); $p.WaitForExit()
}
$raw = (Get-Content -LiteralPath (Join-Path $dir 'questions.txt') | Select-Object -Last 1)
$okEsc = ($raw -match 'Q\|15\|What if a\\\|b and c\\\\d matter\?') -and ($o -match [regex]::Escape($tricky))
$T += [pscustomobject]@{ Name='t17-escaped-fields'; Pass=$okEsc; Fails=@(); Exit=0; Users=3
                        Questions=15; Quizzes=12; Results=5; Files='-'; DataHit=0; Output=$o; Dir=$dir }

# ---------------------------------------------------------------------------
'case                          result  exit  users quest quiz res'
'---------------------------------------------------------------------'
foreach ($c in $T) {
    '{0,-29} {1,-7} {2,-5} {3,-5} {4,-5} {5,-4} {6}' -f $c.Name,
        $(if ($c.Pass) { 'PASS' } else { 'FAIL' }), $c.Exit, $c.Users, $c.Questions, $c.Quizzes, $c.Results
}
''
foreach ($c in $T) { foreach ($f in $c.Fails) { "  !! $($c.Name): $f" } }
$passed = @($T | Where-Object { $_.Pass }).Count
"PASSED $passed / $($T.Count)"