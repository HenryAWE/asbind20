# Run all executables start with "bench_"
Get-ChildItem -Path . -Filter "bench_*.exe" -File | ForEach-Object {
    $exe = $_.FullName
    if (Test-Path $exe) {
        Write-Host "Running benchmark: $exe"
        & $exe
    }
}
