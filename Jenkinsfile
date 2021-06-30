@Library('ctsrd-jenkins-scripts') _

properties([disableConcurrentBuilds(),
            disableResume(),
            [$class: 'GithubProjectProperty', displayName: '', projectUrlStr: 'https://github.com/CTSRD-CHERI/cheri-netperf/'],
            [$class: 'CopyArtifactPermissionProperty', projectNames: '*'],
            [$class: 'JobPropertyImpl', throttle: [count: 3, durationName: 'hour', userBoost: true]],
            pipelineTriggers([githubPush()])
])

jobs = [:]

["riscv64-nocheri", "riscv64-hybrid", "riscv64-purecap"].each { suffix ->
    String name = "netperf-${suffix}"
    jobs[suffix] = { ->
        cheribuildProject(
            target: name,
            capTableABI: "pcrel",
            customGitCheckoutDir: 'netperf',
            architecture: suffix,
            runTests:false,
            tarballName: "${name}.tar.xz")
    }
}

node("linux") {
    jobs.failFast = false
    parallel jobs
}
