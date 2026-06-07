# AI NPC response prototype
# Zone: tutorialb
# NPC: 900903, Sage_Aurelian
#
# Phase 2: asynchronous localhost bridge job.
# EVENT_SAY starts a bridge job, says a short thinking line, then EVENT_TIMER
# polls for the completed in-character response.

BEGIN {
    unshift @INC, "D:/EQEmu/Testbed/server/perl/vendor/lib";
    unshift @INC, "D:/EQEmu/Testbed/server/perl/site/lib";
    unshift @INC, "D:/EQEmu/Testbed/server/perl/lib";
}

use HTTP::Tiny;
use JSON::PP;

our ($client, $name, $npc, $npcid, $text, $timer, $zonesn);

my $AI_BRIDGE_START_URL = "http://127.0.0.1:18080/eqemu/npc-chat/start";
my $AI_BRIDGE_RESULT_URL = "http://127.0.0.1:18080/eqemu/npc-chat/result";
my $AI_START_TIMEOUT_SECONDS = 3;
my $AI_POLL_TIMEOUT_SECONDS = 2;
my $AI_JOB_MAX_AGE_SECONDS = 35;
my $AI_POLL_TIMER = "ai_npc_poll";

my %pending_ai_jobs = ();

sub EVENT_SAY {
    my $enabled = quest::get_rule("CustomFeatures:AiDialogueEnabled");
    if (defined $enabled && $enabled =~ /^(0|false)$/i) {
        quest::say("The old records are quiet right now. Try me again another time.");
        return;
    }

    my $player_message = defined $text ? $text : "";
    $player_message =~ s/^\s+|\s+$//g;
    return if $player_message eq "";

    my $npc_name = "Sage Aurelian";
    if (defined $npc) {
        my $clean_name = eval { $npc->GetCleanName() };
        $npc_name = $clean_name if defined $clean_name && $clean_name ne "";
    }

    my $player_name = defined $name ? $name : "Adventurer";
    my $payload = {
        npc_id => defined $npcid ? $npcid : 900903,
        npc_name => $npc_name,
        zone_short_name => defined $zonesn ? $zonesn : "tutorialb",
        player_name => $player_name,
        player_message => $player_message,
        recent_context => [],
        timeout_seconds => 3.5,
        max_tokens => 72
    };

    my $start = ai_bridge_post_json($AI_BRIDGE_START_URL, $payload, $AI_START_TIMEOUT_SECONDS);
    if (!$start) {
        quest::say("Give me a moment, friend. My thoughts are still forming.");
        return;
    }

    if ($start->{done} && $start->{response}) {
        quest::say(clean_speech($start->{response}));
        return;
    }

    if ($start->{job_id}) {
        $pending_ai_jobs{$start->{job_id}} = {
            player_name => $player_name,
            created_at => time(),
            attempts => 0
        };
        my $ack = $start->{ack_response} || "Hmm. Give me a moment to recall the old records.";
        quest::say(clean_speech($ack));
        quest::settimer($AI_POLL_TIMER, 2);
        return;
    }

    quest::say("The thread is there, but I cannot quite catch it. Ask me again.");
}

sub EVENT_TIMER {
    return if !defined $timer || $timer ne $AI_POLL_TIMER;

    my $now = time();
    my @job_ids = keys %pending_ai_jobs;
    if (!@job_ids) {
        quest::stoptimer($AI_POLL_TIMER);
        return;
    }

    foreach my $job_id (@job_ids) {
        my $job = $pending_ai_jobs{$job_id};
        if (!$job || ($now - $job->{created_at}) > $AI_JOB_MAX_AGE_SECONDS) {
            delete $pending_ai_jobs{$job_id};
            quest::say("The old record slips from my grasp. Ask again, and I will try to catch it.");
            next;
        }

        $job->{attempts}++;
        my $result = ai_bridge_get_json("$AI_BRIDGE_RESULT_URL/$job_id", $AI_POLL_TIMEOUT_SECONDS);
        next if !$result || !$result->{done};

        delete $pending_ai_jobs{$job_id};
        if ($result->{response}) {
            quest::say(clean_speech($result->{response}));
        } else {
            quest::say("I find only scattered rumor. Best not dress it as truth.");
        }
    }

    quest::stoptimer($AI_POLL_TIMER) if !keys %pending_ai_jobs;
}

sub ai_bridge_post_json {
    my ($url, $payload, $timeout) = @_;

    my $json = eval { JSON::PP->new->ascii->canonical->encode($payload) };
    return undef if !$json;

    my $http = HTTP::Tiny->new(
        timeout => $timeout,
        agent => "eqemu-ai-npc-prototype/0.2"
    );

    my $res = eval {
        $http->post(
            $url,
            {
                headers => { "content-type" => "application/json" },
                content => $json
            }
        );
    };
    return undef if !$res || !$res->{success};

    return eval { JSON::PP->new->decode($res->{content}) };
}

sub ai_bridge_get_json {
    my ($url, $timeout) = @_;

    my $http = HTTP::Tiny->new(
        timeout => $timeout,
        agent => "eqemu-ai-npc-prototype/0.2"
    );

    my $res = eval { $http->get($url) };
    return undef if !$res || !$res->{success};

    return eval { JSON::PP->new->decode($res->{content}) };
}

sub clean_speech {
    my ($speech) = @_;
    $speech = "" if !defined $speech;
    $speech =~ s/[\r\n\t]+/ /g;
    $speech =~ s/\s+/ /g;
    $speech =~ s/^\s+|\s+$//g;
    return substr($speech, 0, 420);
}
