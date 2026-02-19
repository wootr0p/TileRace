$enetFile = "external\enet\enet.vcxproj"
$sdlFile = "external\sdl\VisualC\SDL\SDL.vcxproj"

# Fix: Condition='$(Configuration)|$(Platform)'=='X|Y' deve essere
$badCondition = "Condition='`$(Configuration)|`$(Platform)'=="
$goodCondition = "Condition='`$(Configuration)|`$(Platform)'=="

# Questo è già corretto, ma il file ha due virgolette consecutive
# Leggiamo il contenuto e sistemiamo il doppio apice

$enetContent = Get-Content $enetFile -Raw
$sdlContent = Get-Content $sdlFile -Raw

# Il problema è il doppio apice, togliamo uno
$enetContent = $enetContent -replace "Condition='`(.+?)`'`'==", "Condition='`$1'=="
$sdlContent = $sdlContent -replace "Condition='`(.+?)`'`'==", "Condition='`$1'=="

$enetContent | Set-Content $enetFile
$sdlContent | Set-Content $sdlFile

Write-Host "Fix condition done"
