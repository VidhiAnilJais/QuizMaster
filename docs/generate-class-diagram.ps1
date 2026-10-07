# ---------------------------------------------------------------------------
#  Generates docs/class-diagram.png for QuizMaster.
#
#  The diagram mirrors the real classes in include/ - if you add a method or a
#  class, edit the arrays below and run:
#
#      powershell -ExecutionPolicy Bypass -File docs\generate-class-diagram.ps1
#
#  A Mermaid source of the same diagram lives in docs/class-diagram.mmd.
#  Requires only Windows PowerShell 5.1 (System.Drawing), no extra tools.
# ---------------------------------------------------------------------------

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$root   = Split-Path -Parent $PSScriptRoot
$output = Join-Path $root 'docs\class-diagram.png'

$W = 1900
$H = 1600

$bg   = [System.Drawing.Color]::FromArgb(255, 255, 255)
$ink  = [System.Drawing.Color]::FromArgb(28, 42, 66)
$soft = [System.Drawing.Color]::FromArgb(96, 112, 138)
$divd = [System.Drawing.Color]::FromArgb(158, 172, 196)

$fontTitle  = New-Object System.Drawing.Font('Segoe UI', 21, [System.Drawing.FontStyle]::Bold)
$fontSub    = New-Object System.Drawing.Font('Segoe UI', 10.5)
$fontClass  = New-Object System.Drawing.Font('Segoe UI', 13, [System.Drawing.FontStyle]::Bold)
$fontStereo = New-Object System.Drawing.Font('Segoe UI', 9, [System.Drawing.FontStyle]::Italic)
$fontMember = New-Object System.Drawing.Font('Consolas', 10.5)
$fontLabel  = New-Object System.Drawing.Font('Segoe UI', 10, [System.Drawing.FontStyle]::Bold)
$fontNote   = New-Object System.Drawing.Font('Segoe UI', 10)

$brush = New-Object System.Drawing.SolidBrush($ink)
$softB = New-Object System.Drawing.SolidBrush($soft)
$whiteB = New-Object System.Drawing.SolidBrush($bg)

$pen     = New-Object System.Drawing.Pen($ink, 2)
$penMid  = New-Object System.Drawing.Pen($ink, 1.6)
$penSoft = New-Object System.Drawing.Pen($soft, 1.4)
$penDash = New-Object System.Drawing.Pen($soft, 1.5)
$penDash.DashStyle = [System.Drawing.Drawing2D.DashStyle]::Dash

$bmp = New-Object System.Drawing.Bitmap($W, $H)
$g   = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode     = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::ClearTypeGridFit
$g.Clear($bg)

# --- primitives ------------------------------------------------------------

function Draw-Line {
    param([double]$x1, [double]$y1, [double]$x2, [double]$y2, $p = $penMid)
    $g.DrawLine($p, [single]$x1, [single]$y1, [single]$x2, [single]$y2)
}

function Draw-Polyline {
    param([double[]]$Coords, $p = $penMid)
    for ($i = 0; $i -lt $Coords.Count - 2; $i += 2) {
        Draw-Line $Coords[$i] $Coords[$i + 1] $Coords[$i + 2] $Coords[$i + 3] $p
    }
}

function Ptf {
    param([double]$x, [double]$y)
    New-Object System.Drawing.PointF -ArgumentList ([single]$x), ([single]$y)
}

# Open arrow head - plain association / dependency.
function Draw-OpenArrow {
    param([double]$tx, [double]$ty, [double]$fx, [double]$fy)
    $a = [Math]::Atan2($ty - $fy, $tx - $fx)
    $s = 14.0
    Draw-Line $tx $ty ($tx - [Math]::Cos($a - 0.40) * $s) ($ty - [Math]::Sin($a - 0.40) * $s)
    Draw-Line $tx $ty ($tx - [Math]::Cos($a + 0.40) * $s) ($ty - [Math]::Sin($a + 0.40) * $s)
}

# Hollow triangle - UML generalization, points at the base class.
function Draw-Generalization {
    param([double]$tx, [double]$ty, [double]$fx, [double]$fy, [double]$w = 21.0)
    $a = [Math]::Atan2($ty - $fy, $tx - $fx)
    $back = $w * 2.0
    $bx = $tx - [Math]::Cos($a) * $back
    $by = $ty - [Math]::Sin($a) * $back
    $p = $a + [Math]::PI / 2
    $pts = [System.Drawing.PointF[]]@(
        (Ptf $tx $ty),
        (Ptf ($bx + [Math]::Cos($p) * $w / 2) ($by + [Math]::Sin($p) * $w / 2)),
        (Ptf ($bx - [Math]::Cos($p) * $w / 2) ($by - [Math]::Sin($p) * $w / 2))
    )
    $g.FillPolygon($whiteB, $pts)
    $g.DrawPolygon($pen, $pts)
}

# Filled diamond - UML composition, sits on the owning end.
function Draw-Diamond {
    param([double]$tx, [double]$ty, [double]$fx, [double]$fy, [double]$s = 13.0)
    $a = [Math]::Atan2($ty - $fy, $tx - $fx)
    $p = $a + [Math]::PI / 2
    $mx = $tx - [Math]::Cos($a) * $s * 1.7
    $my = $ty - [Math]::Sin($a) * $s * 1.7
    $pts = [System.Drawing.PointF[]]@(
        (Ptf $tx $ty),
        (Ptf ($mx + [Math]::Cos($p) * $s / 2) ($my + [Math]::Sin($p) * $s / 2)),
        (Ptf $mx $my),
        (Ptf ($mx - [Math]::Cos($p) * $s / 2) ($my - [Math]::Sin($p) * $s / 2))
    )
    $g.FillPolygon($brush, $pts)
}

# White disc + redraw of the "over" line = standard UML line jump.
function Draw-LineJump {
    param([double]$cx, [double]$cy, [double]$ax, [double]$ay, [double]$bx, [double]$by)
    $g.FillEllipse($whiteB, [single]($cx - 7), [single]($cy - 7), 14, 14)
    Draw-Line $ax $ay $bx $by $penMid
}

function Label {
    param([string]$Text, [double]$x, [double]$y, $b = $softB, $f = $fontLabel)
    $g.DrawString($Text, $f, $b, [single]$x, [single]$y)
}

# Draws one UML class box and records its geometry in $script:BOX.
function Class-Box {
    param(
        [double]$X, [double]$Y, [double]$BoxW,
        [string]$Name, [string]$Stereotype,
        [string[]]$Attributes, [string[]]$Methods,
        [int[]]$Accent = @(226, 234, 246)
    )
    $headerH = 30
    if ($Stereotype -ne '') { $headerH = 44 }
    $attrH = 10 + $Attributes.Count * 15
    $methH = 10 + $Methods.Count * 15
    $boxH   = $headerH + $attrH + $methH

    $head = New-Object System.Drawing.SolidBrush -ArgumentList ([System.Drawing.Color]::FromArgb($Accent[0], $Accent[1], $Accent[2]))
    $g.FillRectangle($head, [single]$X, [single]$Y, [single]$BoxW, [single]$headerH)
    $head.Dispose()

    $g.DrawRectangle($pen, [single]$X, [single]$Y, [single]$BoxW, [single]$boxH)

    $sep = New-Object System.Drawing.Pen -ArgumentList ($divd, 1)
    $g.DrawLine($sep, [single]$X, [single]($Y + $headerH), [single]($X + $BoxW), [single]($Y + $headerH))
    $g.DrawLine($sep, [single]$X, [single]($Y + $headerH + $attrH), [single]($X + $BoxW), [single]($Y + $headerH + $attrH))
    $sep.Dispose()

    $ns = $g.MeasureString($Name, $fontClass)
    $g.DrawString($Name, $fontClass, $brush, [single]($X + ($BoxW - $ns.Width) / 2), [single]($Y + 5))

    if ($Stereotype -ne '') {
        $ss = $g.MeasureString($Stereotype, $fontStereo)
        $g.DrawString($Stereotype, $fontStereo, $softB, [single]($X + ($BoxW - $ss.Width) / 2), [single]($Y + 24))
    }

    $y = $Y + $headerH + 7
    foreach ($a in $Attributes) { $g.DrawString($a, $fontMember, $brush, [single]($X + 10), [single]$y); $y += 15 }
    foreach ($m in $Methods)    { $g.DrawString($m, $fontMember, $softB, [single]($X + 10), [single]$y); $y += 15 }

    $script:BOX = @{ X = $X; Y = $Y; W = $BoxW; H = $boxH; Bottom = ($Y + $boxH); CX = ($X + $BoxW / 2) }
}

# --- title -----------------------------------------------------------------

$title = 'QuizMaster  -  Class Diagram'
$tw = $g.MeasureString($title, $fontTitle)
$g.DrawString($title, $fontTitle, $brush, [single](($W - $tw.Width) / 2), 20)

$sub = 'Online Quiz Management and Evaluation System     generated from include\*.h by docs\generate-class-diagram.ps1'
$sw = $g.MeasureString($sub, $fontSub)
$g.DrawString($sub, $fontSub, $softB, [single](($W - $sw.Width) / 2), 52)

# --- classes ---------------------------------------------------------------

Class-Box -X 760 -Y 100 -BoxW 400 -Name 'User' -Stereotype '<<abstract>>' -Accent @(255, 231, 212) -Attributes @(
    '- string   username',
    '- string   passwordHash',
    '- bool     loggedIn'
) -Methods @(
    '+ getUsername() const',
    '+ isLoggedIn() const',
    '+ getRole() const           *',
    '+ getDisplayName() const    *',
    '+ showMenu()                *',
    '+ login(username, password) : bool',
    '+ logout()',
    '# setCredential(username, password)',
    '# setLoggedIn(bool)',
    '+ static hashPassword(plain)',
    '+ static loadRecords()',
    '+ static usernameExists(u)',
    '+ static ensureDefaultAdmin()',
    '~ User()   virtual'
)
$userBottom = $script:BOX.Bottom
$userRight  = $script:BOX.X + $script:BOX.W

Class-Box -X 70 -Y 470 -BoxW 410 -Name 'Student' -Stereotype '' -Accent @(226, 240, 226) -Attributes @(
    '- string   studentId',
    '- string   name'
) -Methods @(
    '+ getStudentId() const',
    '+ getName() const',
    '+ getRole() const',
    '+ showMenu()',
    '+ static registerStudent() : bool',
    '+ attemptQuiz()',
    '+ viewResults() const',
    '+ viewAvailableQuizzes() const',
    '+ viewProfile() const',
    '~ Student()'
)
$stuBottom = $script:BOX.Bottom
$stuCX     = $script:BOX.CX

Class-Box -X 1420 -Y 470 -BoxW 410 -Name 'Admin' -Stereotype '' -Accent @(226, 240, 226) -Attributes @(
    '- string             adminId',
    '- string             name',
    '- vector<Question>   questionBank',
    '- vector<Quiz>       quizzes'
) -Methods @(
    '+ getAdminId() const',
    '+ getName() const',
    '+ getRole() const',
    '+ showMenu()',
    '+ addQuestion()',
    '+ updateQuestion()',
    '+ deleteQuestion()',
    '+ createQuiz()',
    '+ viewResults() const',
    '+ quizStatistics() const',
    '+ deleteQuiz()',
    '~ Admin()'
)
$admBottom = $script:BOX.Bottom
$admCX     = $script:BOX.CX

Class-Box -X 70 -Y 880 -BoxW 410 -Name 'Question' -Stereotype '' -Accent @(240, 240, 226) -Attributes @(
    '- int     questionId',
    '- string  questionText',
    '- string  optionA , optionB',
    '- string  optionC , optionD',
    '- char    correctAnswer'
) -Methods @(
    '+ getQuestionId() const',
    '+ getQuestionText() const',
    '+ getOption(letter) const',
    '+ getCorrectAnswer() const',
    '+ isCorrect(answer) const',
    '+ setQuestionText(text)',
    '+ setOption(letter, text)',
    '+ setCorrectAnswer(letter)',
    '+ display() const',
    '+ static normalizeAnswer(raw) : char',
    '+ static loadAll() / saveAll()',
    '~ Question()'
)
$qsnBottom = $script:BOX.Bottom
$qsnRight  = $script:BOX.X + $script:BOX.W

Class-Box -X 760 -Y 880 -BoxW 400 -Name 'Quiz' -Stereotype '' -Accent @(240, 240, 226) -Attributes @(
    '- int                quizId',
    '- string             title',
    '- string             description',
    '- vector<Question>   questions'
) -Methods @(
    '+ getQuizId() const',
    '+ getTitle() const',
    '+ getTotalQuestions() const',
    '+ setTitle(title)',
    '+ addQuestion(question)',
    '+ removeQuestion(id) : bool',
    '+ findQuestion(id) const',
    '+ displayQuestions() const',
    '+ startQuiz(id, name) : Result',
    '+ static loadAll() / saveAll()',
    '~ Quiz()'
)
$quizBottom = $script:BOX.Bottom
$quizLeft   = $script:BOX.X
$quizRight  = $script:BOX.X + $script:BOX.W

Class-Box -X 1420 -Y 880 -BoxW 410 -Name 'Result' -Stereotype '' -Accent @(240, 240, 226) -Attributes @(
    '- int                   resultId',
    '- string                studentId',
    '- string                studentName',
    '- int                   quizId',
    '- string                quizTitle',
    '- string                attemptedAt',
    '- int                   correctAnswers',
    '- int                   totalQuestions',
    '- int                   score',
    '- double                percentage',
    '- vector<AnswerRecord>  answers'
) -Methods @(
    '+ getScore() const',
    '+ getTotal() const',
    '+ getPercentage() const',
    '+ isPassed() : bool',
    '+ getAnswers() const',
    '+ setAnswers(answers)',
    '+ calculateScore()',
    '+ getPerformanceGrade() const',
    '+ getDetailedRows() const',
    '+ displayResult() const',
    '+ saveResult() const',
    '+ static loadAll()',
    '~ Result()'
)
$resBottom = $script:BOX.Bottom
$resCX     = $script:BOX.CX
$resLeft   = $script:BOX.X

# --- relationships ---------------------------------------------------------

# Student -> User (generalization)
Draw-Line $stuCX 470 760 215
Draw-Generalization 760 215 $stuCX 470
Label 'generalization' 400 300

# Admin -> User (generalization)
Draw-Line $admCX 470 $userRight 215
Draw-Generalization $userRight 215 $admCX 470
Label 'generalization' 1215 300

# Student -> Quiz (attempts)
Draw-Line $stuCX $stuBottom $quizLeft 950
Draw-OpenArrow $quizLeft 950 $stuCX $stuBottom
Label 'attempts a quiz' 430 855

# Quiz o-- Question (composition, diamond on the Quiz side)
Draw-Line $quizLeft 1060 $qsnRight 1060
Draw-Diamond $quizLeft 1060 $qsnRight 1060
Draw-OpenArrow $qsnRight 1060 $quizLeft 1060
Label 'owns' 497 1032
Label '1 : 0..*' 497 1050

# Quiz -> Result (creates)
Draw-Line $quizRight 1010 $resLeft 1010
Draw-OpenArrow $resLeft 1010 $quizRight 1010
Label 'creates' 1215 975
Label '1 : 0..*' 1215 993

# Admin -> Quiz (manages)
Draw-Polyline @( $admCX, $admBottom,  $admCX, 815,  960, 815,  960, 880 )
Draw-OpenArrow 960 880 960 815
Label 'manages quizzes' 1290 788

# Admin -> Question (manages the question bank), routed under the middle row
Draw-Polyline @( $resLeft, 560,  1250, 560,  1250, 1330,  275, 1330,  275, $qsnBottom )
Draw-OpenArrow 275 $qsnBottom 275 1330
Label 'manages the question bank' 620 1334

# Student -> Result (owns attempts), routed around the left and bottom margins
Draw-Polyline @( 70, 600,  30, 600,  30, 1385,  $resCX, 1385,  $resCX, $resBottom )
Draw-OpenArrow $resCX $resBottom $resCX 1385
Label 'owns attempts' 880 1388
Label '0..*' 985 1388

# line jump where "manages the question bank" crosses "Quiz -> Result"
Draw-LineJump 1250 1010 $quizRight 1010 $resLeft 1010

# --- support layer ---------------------------------------------------------

$g.DrawRectangle($penDash, [single]55, [single]1405, [single]1790, [single]150)
$g.DrawLine($penDash, [single]55, [single]1405, [single]1330, [single]1405)
Label 'support layer   (utility classes - static members only, never instantiated)' 66 1385

Class-Box -X 90 -Y 1435 -BoxW 375 -Name 'UI' -Stereotype '<<utility>>' -Accent @(236, 236, 236) -Attributes @(
    '- bool   colorEnabled'
) -Methods @(
    '+ static print / header / box / menu',
    '+ static success / error / warning',
    '+ static readInt / readChoice / confirm',
    '+ static readPassword'
)
$script:BOX = $null

Class-Box -X 500 -Y 1435 -BoxW 390 -Name 'FileStore' -Stereotype '<<utility>>' -Accent @(236, 236, 236) -Attributes @(
    '- string   dataDirectory'
) -Methods @(
    '+ static readLines / writeLines',
    '+ static appendLine',
    '+ static escapeField / unescapeField',
    '+ static ensureDataFiles'
)
$script:BOX = $null

Label 'Every domain class renders through UI and persists through FileStore.' 940 1450 $brush $fontNote
Label 'Because both are stateless helpers, no class has to trust std::cin' 940 1476 $softB $fontNote
Label 'and no persistence code is duplicated across the four domain classes.' 940 1500 $softB $fontNote

# --- legend ----------------------------------------------------------------

$g.DrawRectangle($penDash, [single]1300, [single]70, [single]560, [single]212)
Label 'Legend' 1312 52 $brush

Draw-Line 1320 105 1375 105
Draw-Generalization 1375 105 1320 105 18
Label 'inheritance (generalization)' 1390 97

Draw-Line 1320 148 1375 148
Draw-Diamond 1375 148 1320 148 11
Label 'composition (Quiz owns Question)' 1390 140

Draw-Line 1320 191 1375 191
Draw-OpenArrow 1375 191 1320 191
Label 'association / dependency' 1390 183

Draw-Line 1320 234 1375 234 $penDash
Label 'dashed border = utility class' 1390 226

$g.DrawString('+ public    - private    # protected    * pure virtual', $fontMember, $softB, [single]1320, [single]250)
$g.DrawString('User is abstract: getRole(), getDisplayName() and showMenu()', $fontMember, $softB, [single]1320, [single]270)
$g.DrawString('are pure virtual, so User can never be instantiated.', $fontMember, $softB, [single]1320, [single]290)

# ---------------------------------------------------------------------------

$g.Dispose()
$bmp.Save($output, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Host "Class diagram written to: $output"